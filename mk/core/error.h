#ifndef	_CORE_ERROR_H_
#define	_CORE_ERROR_H_

static const char *error_strings[] __used = {
#define	ERROR_IT_S_ALRIGHT	(0x0000)
	"Sterling Void.",
#define	ERROR_NOT_FOUND		(0x0001)
	"Resource not found.",
#define	ERROR_NOT_PERMITTED	(0x0002)
	"Operation not permitted.",
#define	ERROR_EXHAUSTED		(0x0003)
	"Resource exhausted.",
#define	ERROR_NOT_IMPLEMENTED	(0x0004)
	"Lazy programmer.",
#define	ERROR_AGAIN		(0x0005)
	"Try again as needed.",
#define	ERROR_INVALID		(0x0006)
	"Invalid parameter or value.",
#define	ERROR_UNEXPECTED	(0x0007)
	"Unexpected result.",
#define	ERROR_NOT_FREE		(0x0008)
	"Resource in use.",
#define	ERROR_NO_RIGHT		(0x0009)
	"No right to perform operation.",
#define	ERROR_ARG_COUNT		(0x000a)
	"Wrong number of arguments.",
#define	ERROR_COUNT		(0x000b)
};

#endif /* !_CORE_ERROR_H_ */
