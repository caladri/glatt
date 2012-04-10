#ifndef	_FS_FS_OPS_H_
#define	_FS_FS_OPS_H_

/*
 * These are aids to the reader.
 */
typedef	void *fs_context_t;
typedef	void *fs_file_context_t;
typedef	void *fs_directory_context_t;

struct fs_directory_entry {
	char name[1024];
};

typedef	int fs_file_open_op_t(fs_context_t, const char *, fs_file_context_t *) __non_null(2, 3);
typedef	int fs_file_read_op_t(fs_context_t, fs_file_context_t, void *, off_t, size_t *) __non_null(3, 5);
typedef	int fs_file_close_op_t(fs_context_t, fs_file_context_t);

typedef	int fs_directory_open_op_t(fs_context_t, const char *, fs_directory_context_t *) __non_null(2, 3);
typedef	int fs_directory_read_op_t(fs_context_t, fs_directory_context_t, struct fs_directory_entry *, off_t *, size_t *) __non_null(3, 5);
typedef	int fs_directory_close_op_t(fs_context_t, fs_directory_context_t);

struct fs_ops {
	fs_file_open_op_t *fs_file_open;
	fs_file_read_op_t *fs_file_read;
	fs_file_close_op_t *fs_file_close;

	fs_directory_open_op_t *fs_directory_open;
	fs_directory_read_op_t *fs_directory_read;
	fs_directory_close_op_t *fs_directory_close;
};

int fs_register(const char *, struct fs_ops *, fs_context_t) __non_null(1, 2);

#endif /* !_FS_FS_OPS_H_ */
