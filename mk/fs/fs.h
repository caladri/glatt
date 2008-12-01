#ifndef	_FS_FS_H_
#define	_FS_FS_H_

/* Constants.  */

#define	FS_PATH_LENGTH	(65536)
#define	FS_NAME_LENGTH	(FS_PATH_LENGTH / 2)

/* Common types.  */

typedef	uintmax_t fs_cookie_t;
typedef	uint64_t fs_handle_t;
typedef	unsigned char fs_path_t[FS_PATH_LENGTH];
typedef	uint8_t fs_type_t;

/* Well-known values.  */

/* fs_handle_t */
#define	FS_HANDLE_INVALID	((fs_handle_t)~0)

/* fs_type_t */
#define	FS_TYPE_UNKNOWN		(0x00)
#define	FS_TYPE_PORT		(0x01)
#define	FS_TYPE_DIRECTORY	(0x02)

/* Message types.  */

#define	FS_MINIMAL_MESSAGE_FIELDS					\
	fs_cookie_t cookie

#define	FS_HANDLE_MESSAGE_FIELDS					\
	FS_MINIMAL_MESSAGE_FIELDS;					\
	fs_handle_t handle

#define	FS_PATH_MESSAGE_FIELDS						\
	FS_HANDLE_MESSAGE_FIELDS;					\
	fs_path_t path

#define	FS_TYPE_MESSAGE_FIELDS						\
	FS_HANDLE_MESSAGE_FIELDS;					\
	fs_type_t type

#define	FS_MESSAGE(msg, reqtype, resptype)				\
	struct fs_ ## msg ## _request {					\
		_CONCAT(reqtype, _MESSAGE_FIELDS);			\
	};								\
									\
	struct fs_ ## msg ## _response {				\
		_CONCAT(resptype, _MESSAGE_FIELDS);			\
	}

/* Message numbers and structures.  */

#define	FS_MESSAGE_HANDLE_DISPOSE	(0x00000000)

FS_MESSAGE(handle_dispose, FS_HANDLE, FS_HANDLE);

#define	FS_MESSAGE_HANDLE_GET		(0x00000001)

FS_MESSAGE(handle_get, FS_MINIMAL, FS_HANDLE);

#define	FS_MESSAGE_FILE_OPEN		(0x00000002)

FS_MESSAGE(file_open, FS_PATH, FS_TYPE);

#endif /* !_FS_FS_H_ */
