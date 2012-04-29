#include <core/types.h>
#include <core/endian.h>
#include <core/error.h>
#include <core/string.h>
#include <io/storage/device.h>
#include <io/storage/ufs/ufs_directory.h>
#include <io/storage/ufs/ufs_inode.h>
#include <io/storage/ufs/ufs_mount.h>
#include <io/storage/ufs/ufs_param.h>
#include <io/storage/ufs/ufs_superblock.h>
#include <fs/fs_ops.h>
#include <vm/vm.h>
#include <vm/vm_alloc.h>

#include <core/console.h>

#define	ufs_mount_swap64(x)	*(x) = bswap64((uint64_t)*(x))

struct ufs_directory_context {
	struct ufs2_inode d_in;
	struct ufs_directory_entry d_entry;
	uint8_t d_block[UFS_MAX_BSIZE];
	char d_name[256];
};

struct ufs_file_context {
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

static fs_file_open_op_t ufs_op_file_open;
static fs_file_read_op_t ufs_op_file_read;
static fs_file_close_op_t ufs_op_file_close;

static fs_directory_open_op_t ufs_op_directory_open;
static fs_directory_read_op_t ufs_op_directory_read;
static fs_directory_close_op_t ufs_op_directory_close;

static int ufs_lookup(struct ufs_mount *, const char *, uint32_t *);
static int ufs_map_block(struct ufs_mount *, struct ufs2_inode *, off_t, uint64_t *);
static int ufs_read_block(struct ufs_mount *, struct ufs2_inode *, uint64_t, uint8_t *);
static int ufs_read_directory(struct ufs_mount *, struct ufs2_inode *, uint64_t, struct ufs_directory_entry *, char *, size_t);
static int ufs_read_fsbn(struct ufs_mount *, uint64_t, uint8_t *);
static int ufs_read_inode(struct ufs_mount *, uint32_t, struct ufs2_inode *);
static int ufs_read_superblock(struct ufs_mount *);

static struct fs_ops ufs_fs_ops = {
	.fs_file_open = ufs_op_file_open,
	.fs_file_read = ufs_op_file_read,
	.fs_file_close = ufs_op_file_close,

	.fs_directory_open = ufs_op_directory_open,
	.fs_directory_read = ufs_op_directory_read,
	.fs_directory_close = ufs_op_directory_close,
};

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
			panic("%s: vm_free failed: %m", __func__, error);
		return (error);
	}

	error = fs_register("ufs0"/*XXX*/, &ufs_fs_ops, um);
	if (error != 0)
		panic("%s: fs_register failed: %m", __func__, error);

	return (0);
}

static int
ufs_op_file_open(fs_context_t fsc, const char *name, fs_file_context_t *fsfcp)
{
	struct ufs_file_context *fc;
	struct ufs_mount *um = fsc;
	uint32_t inode;
	vaddr_t vaddr;
	int error, error2;

	error = vm_alloc(&kernel_vm, sizeof *fc, &vaddr);
	if (error != 0)
		return (error);
	fc = (struct ufs_file_context *)vaddr;

	error = ufs_lookup(um, name, &inode);
	if (error != 0) {
		error2 = vm_free(&kernel_vm, sizeof *fc, vaddr);
		if (error2 != 0)
			panic("%s: vm_free failed: %m", __func__, error2);

		return (error);
	}

	error = ufs_read_inode(um, inode, &fc->f_in);
	if (error != 0) {
		error2 = vm_free(&kernel_vm, sizeof *fc, vaddr);
		if (error2 != 0)
			panic("%s: vm_free failed: %m", __func__, error2);

		return (error);
	}

	/*
	 * XXX
	 * Check file type.
	 */

	*fsfcp = fc;

	return (0);
}

static int
ufs_op_file_read(fs_context_t fsc, fs_file_context_t fsfc, void *buf, off_t off, size_t *lenp)
{
	struct ufs_file_context *fc = fsfc;
	struct ufs_mount *um = fsc;
	unsigned offset = off % UFS_BSIZE(&um->um_sb);
	uint64_t address = (off >> um->um_sb.sb_bshift) << um->um_sb.sb_bshift;
	size_t len;
	int error;

	if ((uint64_t)off > fc->f_in.in_size)
		return (ERROR_INVALID);

	if ((uint64_t)off == fc->f_in.in_size) {
		*lenp = 0;
		return (0);
	}

	error = ufs_read_block(um, &fc->f_in, address, fc->f_block);
	if (error != 0)
		return (error);

	len = MIN(MIN(*lenp, UFS_BSIZE(&um->um_sb) - offset), fc->f_in.in_size - off);
	memcpy(buf, &fc->f_block[offset], len);
	*lenp = len;

	return (0);
}

static int
ufs_op_file_close(fs_context_t fsc, fs_file_context_t fsfc)
{
	struct ufs_file_context *fc = fsfc;
	int error;

	(void)fsc;

	error = vm_free(&kernel_vm, sizeof *fc, (vaddr_t)fc);
	if (error != 0)
		panic("%s: vm_free failed: %m", __func__, error);

	return (0);
}

static int
ufs_op_directory_open(fs_context_t fsc, const char *name, fs_directory_context_t *fsdcp)
{
	struct ufs_directory_context *dc;
	struct ufs_mount *um = fsc;
	uint32_t inode;
	vaddr_t vaddr;
	int error, error2;

	error = vm_alloc(&kernel_vm, sizeof *dc, &vaddr);
	if (error != 0)
		return (error);
	dc = (struct ufs_directory_context *)vaddr;

	error = ufs_lookup(um, name, &inode);
	if (error != 0) {
		error2 = vm_free(&kernel_vm, sizeof *dc, vaddr);
		if (error2 != 0)
			panic("%s: vm_free failed: %m", __func__, error2);

		return (error);
	}

	error = ufs_read_inode(um, inode, &dc->d_in);
	if (error != 0) {
		error2 = vm_free(&kernel_vm, sizeof *dc, vaddr);
		if (error2 != 0)
			panic("%s: vm_free failed: %m", __func__, error2);

		return (error);
	}

	/*
	 * XXX
	 * Check file type.
	 */

	*fsdcp = dc;

	return (0);
}

static int
ufs_op_directory_read(fs_context_t fsc, fs_directory_context_t fsdc, struct fs_directory_entry *de, off_t *offp, size_t *cntp)
{
	struct ufs_directory_context *dc = fsdc;
	struct ufs_mount *um = fsc;
	uint64_t offset;
	int error;

	offset = *offp;

	if (offset >= dc->d_in.in_size) {
		*offp = 0;
		*cntp = 0;
		return (0);
	}

	error = ufs_read_directory(um, &dc->d_in, offset, &dc->d_entry, de->name, sizeof de->name);
	if (error != 0)
		return (error);

	/* XXX Bounds check; make sure it doesn't span blocks.  */
	*offp = offset + dc->d_entry.de_entrylen;
	*cntp = 1;

	return (0);
}

static int
ufs_op_directory_close(fs_context_t fsc, fs_directory_context_t fsdc)
{
	struct ufs_directory_context *dc = fsdc;
	int error;

	(void)fsc;

	error = vm_free(&kernel_vm, sizeof *dc, (vaddr_t)dc);
	if (error != 0)
		panic("%s: vm_free failed: %m", __func__, error);

	return (0);
}

static int
ufs_lookup(struct ufs_mount *um, const char *path, uint32_t *inodep)
{
	struct ufs_directory_context *dc;
	const char *p, *q;
	uint64_t offset;
	uint32_t inode;
	size_t clen;
	int error;

	if (path[0] != '/')
		return (ERROR_INVALID);

	for (p = path; *p == '/'; p++)
		continue;

	dc = &um->um_dc;
	inode = UFS_ROOT_INODE;

	for (;;) {
		error = ufs_read_inode(um, inode, &dc->d_in);
		if (error != 0)
			return (error);

		if (*p == '\0') {
			*inodep = inode;
			return (0);
		}

		offset = 0;

		q = strchr(p, '/');
		if (q == NULL)
			clen = strlen(p);
		else
			clen = q - p;

		for (;;) {
			if (offset == dc->d_in.in_size)
				return (ERROR_NOT_FOUND);

			error = ufs_read_directory(um, &dc->d_in, offset, &dc->d_entry, dc->d_name, sizeof dc->d_name);
			if (error != 0)
				return (error);

			/* XXX Bounds check; make sure it doesn't span blocks.  */
			offset += dc->d_entry.de_entrylen;

			if (dc->d_entry.de_namelen != clen)
				continue;

			if (strncmp(dc->d_name, p, clen) != 0)
				continue;

			/* We've found our inode.  */
			inode = dc->d_entry.de_inode;

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
ufs_read_directory(struct ufs_mount *um, struct ufs2_inode *in, uint64_t off, struct ufs_directory_entry *de, char *buf, size_t buflen)
{
	unsigned offset = off % UFS_BSIZE(&um->um_sb);
	int error;

	if (offset == 0) {
		error = ufs_read_block(um, in, off, um->um_dc.d_block);
		if (error != 0)
			return (error);
	}

	memcpy(de, &um->um_dc.d_block[offset], sizeof *de);

	if ((um->um_flags & UFS_MOUNT_SWAP) != 0)
		ufs_directory_entry_swap(de);

	strlcpy(buf, (char *)&um->um_dc.d_block[offset + sizeof *de], buflen);

	return (0);
}

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
