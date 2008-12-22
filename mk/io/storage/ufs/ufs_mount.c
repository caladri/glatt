#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/string.h>
#include <io/storage/device.h>
#include <io/storage/ufs/dir.h>
#include <io/storage/ufs/inode.h>
#include <io/storage/ufs/mount.h>
#include <io/storage/ufs/param.h>
#include <io/storage/ufs/superblock.h>
#include <vm/vm.h>
#include <vm/alloc.h>

#include <io/console/console.h>

struct ufs_directory_context {
	uint64_t d_address;
	struct ufs_directory_entry d_entry;
	char d_name[256];
};

struct ufs_mount {
	struct storage_device *um_sdev;

	off_t um_sboff;
	struct ufs_superblock um_sb;

	uint32_t um_inode;
	struct ufs2_inode um_in;

	struct ufs_directory_context um_dc;

	uint8_t um_block[UFS_MAX_BSIZE];

	STAILQ_ENTRY(struct ufs_mount) um_link;
};

static off_t ufs_superblock_offsets[] = UFS_SUPERBLOCK_OFFSETS;
static STAILQ_HEAD(, struct ufs_mount) ufs_mounts =
	STAILQ_HEAD_INITIALIZER(ufs_mounts);

static int ufs_lookup(struct ufs_mount *, const char *, uint32_t *);
static int ufs_map_block(struct ufs_mount *, off_t, uint64_t *);
static int ufs_read_block(struct ufs_mount *, uint64_t);
static int ufs_read_directory(struct ufs_mount *);
static int ufs_read_fsbn(struct ufs_mount *, uint64_t);
static int ufs_read_inode(struct ufs_mount *, uint32_t);
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
	um->um_sdev = sdev;
	um->um_sboff = -1;

	error = ufs_read_superblock(um);
	if (error != 0) {
		error2 = vm_free(&kernel_vm, sizeof *um, umaddr);
		if (error2 != 0)
			panic("%s: vm_free failed: %m", __func__, error);
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

	if (path[0] != '/') {
		return (ERROR_INVALID);
	}

	for (p = path; *p == '/'; p++)
		continue;

	inode = UFS_ROOT_INODE;

	for (;;) {
		error = ufs_read_inode(um, inode);
		if (error != 0)
			return (error);

		q = strchr(p, '/');
		if (q == NULL)
			clen = strlen(p);
		else
			clen = q - p;

		for (;;) {
			if (um->um_dc.d_address == um->um_in.in_size)
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
ufs_map_block(struct ufs_mount *um, off_t logical, uint64_t *blknop)
{
	uint64_t lblock = logical >> um->um_sb.sb_bshift;
	uint64_t iblock, pblock;
	unsigned level, off;
	int error;

	if (lblock < UFS_INODE_NDIRECT) {
		pblock = um->um_in.in_direct[lblock];
		if (pblock == 0) {
			return (ERROR_NOT_FOUND);
		}
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

	panic("using indirect block handling code!");

	if ((iblock = um->um_in.in_indirect[level]) == 0)
		return (ERROR_NOT_FOUND);

	while (level-- != 0) {
		error = ufs_read_fsbn(um, iblock);
		if (error != 0)
			return (error);

		if (level == 0) {
			off = lblock;
		} else {
			off = lblock / UFS_INDIRECTBLOCKS(&um->um_sb, level);
			lblock %= UFS_INDIRECTBLOCKS(&um->um_sb, level);
		}

		memcpy(&iblock, &um->um_block[sizeof iblock * off],
		       sizeof iblock);

		if (iblock == 0)
			return (ERROR_NOT_FOUND);
	}

	*blknop = iblock;
	return (0);
}

static int
ufs_read_block(struct ufs_mount *um, uint64_t off)
{
	uint64_t fsbn;
	int error;

	error = ufs_map_block(um, off, &fsbn);
	if (error != 0)
		return (error);

	error = ufs_read_fsbn(um, fsbn);
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
		error = ufs_read_block(um, um->um_dc.d_address);
		if (error != 0)
			return (error);
	}

	memcpy(de, &um->um_block[offset], sizeof *de);
	strlcpy(um->um_dc.d_name, (char *)&um->um_block[offset + sizeof *de],
		sizeof um->um_dc.d_name);
	/* XXX Bounds check; make sure it doesn't span blocks.  */
	um->um_dc.d_address += de->de_entrylen;

	return (0);
}

static int
ufs_read_fsbn(struct ufs_mount *um, uint64_t fsbn)
{
	uint64_t diskblock;
	unsigned dbshift;
	off_t physical;
	int error;

	diskblock = UFS_FSBN2DBA(&um->um_sb, fsbn);
	dbshift = um->um_sb.sb_fshift - um->um_sb.sb_fsbtodb;
	physical = diskblock << dbshift;

	error = storage_device_read(um->um_sdev, um->um_block,
				    UFS_BSIZE(&um->um_sb), physical);
	if (error != 0)
		return (error);

	return (0);
}

static int
ufs_read_inode(struct ufs_mount *um, uint32_t inode)
{
	unsigned i = inode % um->um_sb.sb_inopb;
	int error;

	error = ufs_read_fsbn(um, ufs_inode_block(&um->um_sb, inode));
	if (error != 0)
		return (error);

	um->um_dc.d_address = 0;
	um->um_inode = inode;
	memcpy(&um->um_in, &um->um_block[i * sizeof um->um_in],
	       sizeof um->um_in);

	return (0);
}

static int
ufs_read_superblock(struct ufs_mount *um)
{
	unsigned i;
	int error;

	for (i = 0; ufs_superblock_offsets[i] != -1; i++) {
		error = storage_device_read(um->um_sdev, um->um_block,
					    UFS_SUPERBLOCK_SIZE,
					    ufs_superblock_offsets[i]);
		if (error != 0)
			continue;

		/* Check superblock.  */
		memcpy(&um->um_sb, um->um_block, sizeof um->um_sb);

		switch (um->um_sb.sb_magic) {
		case UFS_SUPERBLOCK_MAGIC1:
		case UFS_SUPERBLOCK_SWAP1:
			return (ERROR_NOT_IMPLEMENTED);
		case UFS_SUPERBLOCK_MAGIC2:
			break;
		case UFS_SUPERBLOCK_SWAP2:
			/* XXX Do I want to do byte-swapping?  */
			return (ERROR_INVALID);
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
	struct ufs_mount *um;
	uint32_t inode;
	int error;

	STAILQ_FOREACH(um, &ufs_mounts, um_link) {
		error = ufs_lookup(um, "/mu/servers/", &inode);
		if (error != 0)
			continue;

		error = ufs_read_inode(um, inode);
		if (error != 0)
			continue;

		for (;;) {
			if (um->um_dc.d_address == um->um_in.in_size)
				break;

			error = ufs_read_directory(um);
			if (error != 0)
				break;

			kcprintf("autorun %s\n", um->um_dc.d_name);
		}
	}
}
STARTUP_ITEM(ufs_autorun, STARTUP_SERVERS, STARTUP_SECOND, ufs_autorun, NULL);
