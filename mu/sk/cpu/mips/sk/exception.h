#ifndef	_CPU_SK_EXCEPTION_H_
#define	_CPU_SK_EXCEPTION_H_

#define	__VECTOR_ENTRY(name)	name ## VectorEntry
#define	__VECTOR_END(name)	name ## VectorEnd

#define	VECTOR_ENTRY(name)						\
	ENTRY(__VECTOR_ENTRY(name))
#define	VECTOR_END(name)						\
	SYMBOL(__VECTOR_END(name))					\
	END(__VECTOR_ENTRY(name))

#endif /* !_CPU_SK_EXCEPTION_H_ */
