#ifndef	_FS_FS_TREE_H_
#define	_FS_FS_TREE_H_

struct fs_directory;

struct fs_file {
	struct fs_directory *f_parent;
	enum fs_file_type f_type;
	char f_name[FS_NAME_LENGTH];

	union {
		ipc_port_t fu_port;
		struct fs_directory *fu_directory;
#if 0
		struct fs_file *fu_target;
#endif
	} f_union;
};

struct fs_directory {
	struct fs_file d_self;
	ipc_port_t d_port;
	SLIST_ENTRY(struct fs_file) d_files;
};

#endif /* !_FS_FS_TREE_H_ */
