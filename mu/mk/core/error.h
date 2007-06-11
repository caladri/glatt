#ifndef	_CORE_ERROR_H_
#define	_CORE_ERROR_H_

static const char *error_strings[] = {
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
#define	ERROR_COUNT		(0x0007)
};
COMPILE_TIME_ASSERT(sizeof error_strings / sizeof error_strings[0] ==
		    ERROR_COUNT);

#endif /* !_CORE_ERROR_H_ */
