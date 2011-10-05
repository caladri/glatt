#include <core/types.h>
#include <core/endian.h>
#include <core/error.h>
#ifdef EXEC
#include <core/exec.h>
#endif
#include <core/startup.h>
#include <core/string.h>
#include <io/storage/device.h>
#include <io/storage/ufs/ufs_directory.h>
#include <io/storage/ufs/ufs_inode.h>
#include <io/storage/ufs/ufs_mount.h>
#include <io/storage/ufs/ufs_param.h>
#include <io/storage/ufs/ufs_superblock.h>
#include <vm/vm.h>
#include <vm/vm_alloc.h>

#include <io/console/console.h>

#define	ufs_mount_swap64(x)	*(x) = bswap64((uint64_t)*(x))

struct ufs_directory_context {
	struct ufs2_inode d_in;
	uint64_t d_address;
	struct ufs_directory_entry d_entry;
	uint8_t d_block[UFS_MAX_BSIZE];
	char d_name[256];
};

struct ufs_file_context {
	struct ufs_mount *f_um;
	struct ufs2_inode f_in;
	uint8_t f_block[UFS_MAX_BSIZE];
};

struct ufs_mount {
	unsigned um_flags;

	struct storage_device *um_sdev;

	off_t um_sboff;
	struct ufs_superblock um_sb;

	struct ufs_directory_context um_dc;

	uint8_t um_iblock[UFS_MAX_BSIZE];

	STAILQ_ENTRY(struct ufs_mount) um_link;
};

#define	UFS_MOUNT_DEFAULT	(0x00000000)
#define	UFS_MOUNT_SWAP		(0x00000001)

static off_t ufs_superblock_offsets[] = UFS_SUPERBLOCK_OFFSETS;
static STAILQ_HEAD(, struct ufs_mount) ufs_mounts =
	STAILQ_HEAD_INITIALIZER(ufs_mounts);

static int ufs_lookup(struct ufs_mount *, const char *, uint32_t *);
static int ufs_map_block(struct ufs_mount *, struct ufs2_inode *, off_t, uint64_t *);
static int ufs_read_block(struct ufs_mount *, struct ufs2_inode *, uint64_t, uint8_t *);
static int ufs_read_directory(struct ufs_mount *);
#ifdef EXEC
static int ufs_read_file(void *, void *, off_t, size_t *);
#endif
static int ufs_read_fsbn(struct ufs_mount *, uint64_t, uint8_t *);
static int ufs_read_inode(struct ufs_mount *, uint32_t, struct ufs2_inode *);
static int ufs_read_superblock(struct ufs_mount *);

int
ufs_mount(struct storage_device *sdev)
{
	struct ufs_mount *um;
	vaddr_t umaddr;
	int error, error2;

	error = vm_alloc(&kernel_vm, sizeof *um, &umaddr);
	if (error != 0)
		return (error);

	um = (struct ufs_mount *)umaddr;
	um->um_flags = UFS_MOUNT_DEFAULT;
	um->um_sdev = sdev;
	um->um_sboff = -1;

	error = ufs_read_superblock(um);
	if (error != 0) {
		error2 = vm_free(&kernel_vm, sizeof *um, umaddr);
		if (error2 != 0)
			panic("%s: vm_free failed: %m\n", __func__, error);
		return (error);
	}

	STAILQ_INSERT_TAIL(&ufs_mounts, um, um_link);

	return (0);
}

static int
ufs_lookup(struct ufs_mount *um, const char *path, uint32_t *inodep)
{
	const char *p, *q;
	uint32_t inode;
	size_t clen;
	int error;

	if (path[0] != '/')
		return (ERROR_INVALID);

	for (p = path; *p == '/'; p++)
		continue;

	inode = UFS_ROOT_INODE;

	for (;;) {
		error = ufs_read_inode(um, inode, &um->um_dc.d_in);
		if (error != 0)
			return (error);

		um->um_dc.d_address = 0;

		q = strchr(p, '/');
		if (q == NULL)
			clen = strlen(p);
		else
			clen = q - p;

		for (;;) {
			if (um->um_dc.d_address == um->um_dc.d_in.in_size)
				return (ERROR_NOT_FOUND);

			error = ufs_read_directory(um);
			if (error != 0)
				return (error);

			if (um->um_dc.d_entry.de_namelen != clen)
				continue;

			if (strncmp(um->um_dc.d_name, p, clen) != 0)
				continue;

			/* We've found our inode.  */
			inode = um->um_dc.d_entry.de_inode;

			p += clen;

			/* XXX Check if this is a directory.  */
			while (*p == '/')
				p++;

			/* Now look for the next component under inode.  */
			if (*p != '\0')
				break;

			*inodep = inode;

			return (0);
		}
	}
	NOTREACHED();
}

static int
ufs_map_block(struct ufs_mount *um, struct ufs2_inode *in, off_t logical, uint64_t *blknop)
{
	uint64_t lblock = logical >> um->um_sb.sb_bshift;
	uint64_t iblock, pblock;
	unsigned level, off;
	int error;

	if (lblock < UFS_INODE_NDIRECT) {
		pblock = in->in_direct[lblock];
		if (pblock == 0)
			return (ERROR_NOT_FOUND);
		*blknop = pblock;
		return (0);
	}

	lblock -= UFS_INODE_NDIRECT;

	for (level = 0; level < UFS_INODE_NINDIRECT; level++) {
		uint64_t blocks = UFS_INDIRECTBLOCKS(&um->um_sb, level);
		if (lblock < blocks)
			break;
		lblock -= blocks;
	}

	if (level != 0)
		panic("using untested indirect block handling code!");

	if ((iblock = in->in_indirect[level]) == 0)
		return (ERROR_NOT_FOUND);

	for (;;) {
		if (level == 0) {
			off = lblock;
		} else {
			off = lblock / UFS_INDIRECTBLOCKS(&um->um_sb, level);
			lblock %= UFS_INDIRECTBLOCKS(&um->um_sb, level);
		}

		error = ufs_read_fsbn(um, iblock, um->um_iblock);
		if (error != 0)
			return (error);

		memcpy(&iblock, &um->um_iblock[sizeof iblock * off],
		       sizeof iblock);

		if ((um->um_flags & UFS_MOUNT_SWAP) != 0)
			ufs_mount_swap64(&iblock);

		if (iblock == 0)
			return (ERROR_NOT_FOUND);

		if (level-- == 0) {
			*blknop = iblock;
			return (0);
		}
	}
}

static int
ufs_read_block(struct ufs_mount *um, struct ufs2_inode *in, uint64_t off, uint8_t *buf)
{
	uint64_t fsbn;
	int error;

	error = ufs_map_block(um, in, off, &fsbn);
	if (error != 0)
		return (error);

	error = ufs_read_fsbn(um, fsbn, buf);
	if (error != 0)
		return (error);

	return (0);
}

static int
ufs_read_directory(struct ufs_mount *um)
{
	unsigned offset = um->um_dc.d_address % UFS_BSIZE(&um->um_sb);
	struct ufs_directory_entry *de = &um->um_dc.d_entry;
	int error;

	if (offset == 0) {
		error = ufs_read_block(um, &um->um_dc.d_in, um->um_dc.d_address, um->um_dc.d_block);
		if (error != 0)
			return (error);
	}

	memcpy(de, &um->um_dc.d_block[offset], sizeof *de);

	if ((um->um_flags & UFS_MOUNT_SWAP) != 0)
		ufs_directory_entry_swap(de);

	strlcpy(um->um_dc.d_name, (char *)&um->um_dc.d_block[offset + sizeof *de],
		sizeof um->um_dc.d_name);
	/* XXX Bounds check; make sure it doesn't span blocks.  */
	um->um_dc.d_address += de->de_entrylen;

	return (0);
}

#ifdef EXEC
static int
ufs_read_file(void *softc, void *buf, off_t off, size_t *lenp)
{
	struct ufs_file_context *fc = softc;
	struct ufs_mount *um = fc->f_um;
	unsigned offset = off % UFS_BSIZE(&um->um_sb);
	uint64_t address = (off >> um->um_sb.sb_bshift) << um->um_sb.sb_bshift;
	size_t len;
	int error;

	error = ufs_read_block(um, &fc->f_in, address, fc->f_block);
	if (error != 0)
		return (error);

	len = MIN(*lenp, UFS_BSIZE(&um->um_sb) - offset);
	memcpy(buf, &fc->f_block[offset], len);
	*lenp = len;

	return (0);
}
#endif

static int
ufs_read_fsbn(struct ufs_mount *um, uint64_t fsbn, uint8_t *buf)
{
	uint64_t diskblock;
	unsigned dbshift;
	off_t physical;
	int error;

	diskblock = UFS_FSBN2DBA(&um->um_sb, fsbn);
	dbshift = um->um_sb.sb_fshift - um->um_sb.sb_fsbtodb;
	physical = diskblock << dbshift;

	error = storage_device_read(um->um_sdev, buf,
				    UFS_BSIZE(&um->um_sb), physical);
	if (error != 0)
		return (error);

	return (0);
}

static int
ufs_read_inode(struct ufs_mount *um, uint32_t inode, struct ufs2_inode *in)
{
	unsigned i = inode % UFS_INOPB(&um->um_sb);
	int error;

	error = ufs_read_fsbn(um, ufs_inode_block(&um->um_sb, inode), um->um_iblock);
	if (error != 0)
		return (error);

	memcpy(in, &um->um_iblock[i * sizeof *in], sizeof *in);

	if ((um->um_flags & UFS_MOUNT_SWAP) != 0)
		ufs_inode_swap(in);

	return (0);
}

static int
ufs_read_superblock(struct ufs_mount *um)
{
	unsigned i;
	int error;

	for (i = 0; ufs_superblock_offsets[i] != -1; i++) {
		error = storage_device_read(um->um_sdev, um->um_iblock,
					    UFS_SUPERBLOCK_SIZE,
					    ufs_superblock_offsets[i]);
		if (error != 0)
			continue;

		/* Check superblock.  */
		memcpy(&um->um_sb, um->um_iblock, sizeof um->um_sb);

		switch (um->um_sb.sb_magic) {
		case UFS_SUPERBLOCK_MAGIC1:
		case UFS_SUPERBLOCK_SWAP1:
			return (ERROR_NOT_IMPLEMENTED);
		case UFS_SUPERBLOCK_MAGIC2:
			break;
		case UFS_SUPERBLOCK_SWAP2:
			um->um_flags |= UFS_MOUNT_SWAP;
			ufs_superblock_swap(&um->um_sb);
			break;
		default:
			continue;
		}

		if (LOG2(UFS_MAX_BSIZE) < um->um_sb.sb_bshift)
			return (ERROR_NOT_IMPLEMENTED);

		um->um_sboff = ufs_superblock_offsets[i];
		break;
	}

	if (um->um_sboff == -1)
		return (ERROR_NOT_FOUND);

	return (0);
}

static void
ufs_autorun(void *arg)
{
#ifdef EXEC
	struct ufs_file_context *fc;
	vaddr_t vaddr;
#endif
	struct ufs_mount *um;
	uint32_t inode;
	int error;

	if (STAILQ_EMPTY(&ufs_mounts))
		return;

#ifdef EXEC
	error = vm_alloc(&kernel_vm, sizeof *fc, &vaddr);
	if (error != 0) {
		kcprintf("%s: vm_alloc failed: %m\n", __func__, error);
		return;
	}

	fc = (struct ufs_file_context *)vaddr;
#endif

	STAILQ_FOREACH(um, &ufs_mounts, um_link) {
#ifdef EXEC
		fc->f_um = um;
#endif

		error = ufs_lookup(um, "/mu/servers/", &inode);
		if (error != 0) {
			kcprintf("%s: ufs_lookup failed: %m\n", __func__, error);
			continue;
		}

		error = ufs_read_inode(um, inode, &um->um_dc.d_in);
		if (error != 0)
			continue;

		um->um_dc.d_address = 0;

		for (;;) {
			if (um->um_dc.d_address == um->um_dc.d_in.in_size)
				break;

			error = ufs_read_directory(um);
			if (error != 0)
				break;

			if (um->um_dc.d_name[0] == '.')
				continue;
			kcprintf("autorun %s\n", um->um_dc.d_name);

			/*
			 * XXX
			 * Check that this is a regular, executable file.
			 */

#ifdef EXEC
			error = ufs_read_inode(um, um->um_dc.d_entry.de_inode, &fc->f_in);
			if (error != 0) {
				kcprintf("%s: ufs_read_inode failed: %m\n", __func__, error);
				continue;
			}

			error = exec_task(um->um_dc.d_name, ufs_read_file, fc);
			if (error != 0) {
				kcprintf("%s: exec_task failed: %m\n", __func__, error);
				continue;
			}
#endif
		}
	}

#ifdef EXEC
	error = vm_free(&kernel_vm, sizeof *fc, vaddr);
	if (error != 0)
		panic("%s: vm_free failed: %m", __func__, error);
#endif
}
STARTUP_ITEM(ufs_autorun, STARTUP_SERVERS, STARTUP_SECOND, ufs_autorun, NULL);
