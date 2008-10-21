#ifndef	_CORE_ASSERT_H_
#define	_CORE_ASSERT_H_

#define	ASSERT(p, s)							\
	if (!(p))							\
		panic("%s:%u (in %s): %s failed: %s",			\
		      __FILE__, __LINE__, __func__, #p, s)

#define	NOTREACHED()	ASSERT(false, "Should not be reached.");

void panic(const char *, ...) __noreturn/*XXX %m __printf(1, 2)*/;

#endif /* !_CORE_ASSERT_H_ */
