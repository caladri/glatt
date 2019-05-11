#include <core/types.h>
#include <core/endian.h>
#include <core/error.h>
#include <core/string.h>
#include <io/storage/device.h>
#include <io/storage/tarfs/tarfs_mount.h>
#include <fs/fs.h>
#include <fs/fs_ops.h>
#include <vm/vm.h>
#include <vm/vm_alloc.h>
#include <vm/vm_page.h>

#define	TARFS_BSIZE			(512)
#define	TARFS_MAX_NAME_LENGTH		(100)
#define	TARFS_MAX_NAME_COMPONENTS	(TARFS_MAX_NAME_LENGTH / 2)

struct tar_header {
	uint8_t th_name[100];
	uint8_t th_mode[8];
	uint8_t th_uid[8];
	uint8_t th_gid[8];
	uint8_t th_size[12];
	uint8_t th_mtime[12];
	uint8_t th_chksum[8];
	uint8_t th_typeflag;
	uint8_t th_linkname[100];
	uint8_t th_magic[6];
	uint8_t th_version[2];
	uint8_t th_uname[32];
	uint8_t th_gname[32];
	uint8_t th_devmajor[8];
	uint8_t th_devminor[8];
	uint8_t th_prefix[155];
};

static bool
tar_header_decode(const uint8_t *field, size_t fieldlen, uint64_t *fieldvalp)
{
	uint64_t fieldval;

	fieldval = 0;

	while (fieldlen != 0 && *field != ' ') {
		if (*field < '0' || *field > '9')
			return (false);
		fieldval *= 8;
		fieldval |= *field - '0';

		fieldlen--;
		field++;
	}

	*fieldvalp = fieldval;

	return (true);
}

static const uint8_t tar_header_magic[6] = "ustar";
static const uint8_t tar_header_magic_zero[6];

struct tarfs_path {
	char tp_path[TARFS_MAX_NAME_LENGTH];
	const char *tp_names[TARFS_MAX_NAME_COMPONENTS];
	size_t tp_namecnt;
};

struct tarfs_file_context {
	struct tar_header f_header;
	uint64_t f_daddr;
	uint64_t f_dsize;

	struct tarfs_path f_path;

	uint8_t f_scratch[PAGE_SIZE]; /* XXX */
};

struct tarfs_mount {
	unsigned tm_flags;

	struct storage_device *tm_sdev;

	uint8_t tm_block[TARFS_BSIZE];

	struct tar_header tm_header;

	struct tarfs_path tm_match;

	STAILQ_ENTRY(struct tarfs_mount) tm_link;

	uint8_t tm_scratch[PAGE_SIZE]; /* XXX */
};

#define	TARFS_MOUNT_DEFAULT	(0x00000000)

static fs_file_open_op_t tarfs_op_file_open;
static fs_file_read_op_t tarfs_op_file_read;
static fs_file_close_op_t tarfs_op_file_close;

static fs_directory_open_op_t tarfs_op_directory_open;
static fs_directory_read_op_t tarfs_op_directory_read;
static fs_directory_close_op_t tarfs_op_directory_close;

static int tarfs_lookup(struct tarfs_mount *, const char *, struct tarfs_file_context *);
static bool tarfs_lookup_match(struct tarfs_mount *, const struct tarfs_file_context *, const uint8_t *, size_t, bool);
static int tarfs_read_header(struct tarfs_mount *, uint64_t, bool *);
static int tarfs_split_path(struct tarfs_path *, const void *, size_t);

static struct fs_ops tarfs_fs_ops = {
	.fs_file_open = tarfs_op_file_open,
	.fs_file_read = tarfs_op_file_read,
	.fs_file_close = tarfs_op_file_close,

	.fs_directory_open = tarfs_op_directory_open,
	.fs_directory_read = tarfs_op_directory_read,
	.fs_directory_close = tarfs_op_directory_close,
};

int
tarfs_mount(struct storage_device *sdev)
{
	struct tarfs_mount *tm;
	vaddr_t tmaddr;
	int error, error2;

	error = vm_alloc(&kernel_vm, sizeof *tm, &tmaddr, VM_ALLOC_DEFAULT);
	if (error != 0)
		return (error);

	tm = (struct tarfs_mount *)tmaddr;
	tm->tm_flags = TARFS_MOUNT_DEFAULT;
	tm->tm_sdev = sdev;

	error = tarfs_read_header(tm, 0, NULL);
	if (error != 0) {
		error2 = vm_free(&kernel_vm, sizeof *tm, tmaddr);
		if (error2 != 0)
			panic("%s: vm_free failed: %m", __func__, error);
		return (error);
	}

	error = fs_register("ufs0"/*XXX*/, &tarfs_fs_ops, tm);
	if (error != 0)
		panic("%s: fs_register failed: %m", __func__, error);

	return (0);
}

static int
tarfs_op_file_open(fs_context_t fsc, const char *name, fs_file_context_t *fsfcp)
{
	struct tarfs_file_context *fc;
	struct tarfs_mount *tm = fsc;
	vaddr_t vaddr;
	int error, error2;

	error = vm_alloc(&kernel_vm, sizeof *fc, &vaddr, VM_ALLOC_DEFAULT);
	if (error != 0)
		return (error);
	fc = (struct tarfs_file_context *)vaddr;

	error = tarfs_lookup(tm, name, fc);
	if (error != 0) {
		error2 = vm_free(&kernel_vm, sizeof *fc, vaddr);
		if (error2 != 0)
			panic("%s: vm_free failed: %m", __func__, error2);

		return (error);
	}

	/*
	 * Check file type.
	 */
	if (fc->f_header.th_typeflag != '0') {
		error = vm_free(&kernel_vm, sizeof *fc, vaddr);
		if (error != 0)
			panic("%s: vm_free failed: %m", __func__, error);

		return (ERROR_WRONG_KIND);
	}

	*fsfcp = fc;

	return (0);
}

static int
tarfs_op_file_read(fs_context_t fsc, fs_file_context_t fsfc, void *buf, off_t off, size_t *lenp)
{
	struct tarfs_file_context *fc = fsfc;
	struct tarfs_mount *tm = fsc;
	unsigned offset = off % TARFS_BSIZE;
	uint64_t address = off - offset;
	size_t len;
	int error;

	if ((uint64_t)off > fc->f_dsize)
		return (ERROR_INVALID);

	if ((uint64_t)off == fc->f_dsize) {
		*lenp = 0;
		return (0);
	}

	error = storage_device_read(tm->tm_sdev, tm->tm_block,
				    sizeof tm->tm_block, fc->f_daddr + address);
	if (error != 0)
		return (error);

	len = MIN(MIN(*lenp, TARFS_BSIZE - offset), fc->f_dsize - off);
	memcpy(buf, &tm->tm_block[offset], len);
	*lenp = len;

	return (0);
}

static int
tarfs_op_file_close(fs_context_t fsc, fs_file_context_t fsfc)
{
	struct tarfs_file_context *fc = fsfc;
	int error;

	(void)fsc;

	error = vm_free(&kernel_vm, sizeof *fc, (vaddr_t)fc);
	if (error != 0)
		panic("%s: vm_free failed: %m", __func__, error);

	return (0);
}

static int
tarfs_op_directory_open(fs_context_t fsc, const char *name, fs_directory_context_t *fsdcp)
{
	struct tarfs_file_context *dc;
	struct tarfs_mount *tm = fsc;
	vaddr_t vaddr;
	int error, error2;

	error = vm_alloc(&kernel_vm, sizeof *dc, &vaddr, VM_ALLOC_DEFAULT);
	if (error != 0)
		return (error);
	dc = (struct tarfs_file_context *)vaddr;

	error = tarfs_lookup(tm, name, dc);
	if (error != 0) {
		error2 = vm_free(&kernel_vm, sizeof *dc, vaddr);
		if (error2 != 0)
			panic("%s: vm_free failed: %m", __func__, error2);

		return (error);
	}

	/*
	 * Check file type.
	 */
	if (dc->f_header.th_typeflag != '5') {
		error = vm_free(&kernel_vm, sizeof *dc, vaddr);
		if (error != 0)
			panic("%s: vm_free failed: %m", __func__, error);

		return (ERROR_WRONG_KIND);
	}

	*fsdcp = dc;

	return (0);
}

static int
tarfs_op_directory_read(fs_context_t fsc, fs_directory_context_t fsdc, struct fs_directory_entry *de, off_t *offp, size_t *cntp)
{
	struct tarfs_file_context *dc = fsdc;
	struct tarfs_mount *tm = fsc;
	uint64_t offset, skip, dsize;
	int error;
	bool eof;

	skip = *offp;

	offset = dc->f_daddr + ROUNDUP(dc->f_dsize, TARFS_BSIZE);
	for (;;) {
		error = tarfs_read_header(tm, offset, &eof);
		if (error != 0)
			return (error);

		if (eof) {
			*offp = 0;
			*cntp = 0;
			return (0);
		}

		if (!tar_header_decode(tm->tm_header.th_size, sizeof tm->tm_header.th_size, &dsize))
			return (ERROR_INVALID);

		if (tarfs_lookup_match(tm, dc, tm->tm_header.th_name, sizeof tm->tm_header.th_name, true)) {
			if (skip == 0)
				break;
			skip--;
		}
		offset += TARFS_BSIZE + ROUNDUP(dsize, TARFS_BSIZE); /* Skip header block and data size.  */
	}

	if (tm->tm_match.tp_namecnt == 0)
		return (ERROR_INVALID);
	strlcpy(de->name, tm->tm_match.tp_names[tm->tm_match.tp_namecnt - 1], sizeof de->name); /* Yield last component of name.  */
	*offp = *offp + 1;
	*cntp = 1;
	return (0);
}

static int
tarfs_op_directory_close(fs_context_t fsc, fs_directory_context_t fsdc)
{
	struct tarfs_file_context *dc = fsdc;
	int error;

	(void)fsc;

	error = vm_free(&kernel_vm, sizeof *dc, (vaddr_t)dc);
	if (error != 0)
		panic("%s: vm_free failed: %m", __func__, error);

	return (0);
}

static int
tarfs_lookup(struct tarfs_mount *tm, const char *path, struct tarfs_file_context *fc)
{
	uint64_t offset, dsize;
	int error;

	if (path[0] != '/')
		return (ERROR_INVALID);

	error = tarfs_split_path(&fc->f_path, path, strlen(path));
	if (error != 0)
		return (error);

	offset = 0;
	for (;;) {
		error = tarfs_read_header(tm, offset, NULL);
		if (error != 0)
			return (error);

		if (!tar_header_decode(tm->tm_header.th_size, sizeof tm->tm_header.th_size, &dsize))
			return (ERROR_INVALID);

		if (!tarfs_lookup_match(tm, fc, tm->tm_header.th_name, sizeof tm->tm_header.th_name, false)) {
			offset += TARFS_BSIZE + ROUNDUP(dsize, TARFS_BSIZE); /* Skip header block and data size.  */
			continue;
		}

		memcpy(&fc->f_header, &tm->tm_header, sizeof fc->f_header);
		fc->f_daddr = offset + TARFS_BSIZE;
		fc->f_dsize = dsize;
		return (0);
	}
	NOTREACHED();
}

static bool
tarfs_lookup_match(struct tarfs_mount *tm, const struct tarfs_file_context *fc, const uint8_t *name, size_t namelen, bool children)
{
	unsigned i;
	int error;

	/* XXX Flag error?  */
	error = tarfs_split_path(&tm->tm_match, name, namelen);
	if (error != 0)
		return (false);

	/*
	 * If we are looking for children of a directory, we expect to have
	 * exactly one more name component.
	 */
	if (children) {
		if (tm->tm_match.tp_namecnt == 0 ||
		    tm->tm_match.tp_namecnt - 1 != fc->f_path.tp_namecnt)
			return (false);
	} else {
		if (tm->tm_match.tp_namecnt != fc->f_path.tp_namecnt)
			return (false);
	}

	/*
	 * Match all components of the file or parent directory name.
	 */
	for (i = 0; i < fc->f_path.tp_namecnt; i++) {
		if (strcmp(tm->tm_match.tp_names[i], fc->f_path.tp_names[i]) != 0)
			return (false);
	}

	return (true);
}

static int
tarfs_read_header(struct tarfs_mount *tm, uint64_t offset, bool *eofp)
{
	int error;

	if (offset % TARFS_BSIZE != 0)
		return (ERROR_INVALID);

	error = storage_device_read(tm->tm_sdev, tm->tm_block,
				    sizeof tm->tm_block, offset);
	if (error != 0)
		return (error);

	memcpy(&tm->tm_header, tm->tm_block, sizeof tm->tm_header);

	if (memcmp(tm->tm_header.th_magic, tar_header_magic, sizeof tar_header_magic) != 0) {
		/*
		 * If the magic is all zeroes, and we want EOF notification,
		 * return no error and indicate EOF.
		 */
		if (memcmp(tm->tm_header.th_magic, tar_header_magic_zero, sizeof tar_header_magic_zero) == 0) {
			if (eofp != NULL) {
				*eofp = true;
				return (0);
			}
			return (ERROR_NOT_FOUND);
		}
		return (ERROR_INVALID);
	}

	/* XXX Do other validation.  */

	if (eofp != NULL)
		*eofp = false;

	return (0);
}

static int
tarfs_split_path(struct tarfs_path *tp, const void *path, size_t pathlen)
{
	char *p, *q;

	if (strlcpy(tp->tp_path, path, sizeof tp->tp_path) >= sizeof tp->tp_path)
		return (ERROR_INVALID);

	tp->tp_namecnt = 0;
	p = tp->tp_path;
	while (*p != '\0') {
		/* Skip slashes.  */
		while (*p == '/') {
			p++;
		}

		if (*p == '\0')
			break;

		/* Save off and NUL-terminate this component.  */
		q = strchr(p, '/');
		if (q != NULL)
			*q = '\0';
		/* Skip "." entries/components.  */
		if (strcmp(p, ".") != 0)
			tp->tp_names[tp->tp_namecnt++] = p;
		if (q == NULL)
			break;
		p = q + 1;
	}

	return (0);
}
