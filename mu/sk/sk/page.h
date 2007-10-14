#ifndef	_SK_PAGE_H_
#define	_SK_PAGE_H_

#include <cpu/sk/page.h>

#define	PAGE_SHIFT	(LOG2(PAGE_SIZE))
#define	PAGE_MASK	(PAGE_SIZE - 1)

COMPILE_TIME_ASSERT(1 << PAGE_SHIFT == PAGE_SIZE);

#endif /* !_SK_PAGE_H_ */
