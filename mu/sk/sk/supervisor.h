#ifndef	_SK_SUPERVISOR_H_
#define	_SK_SUPERVISOR_H_

#define	Supervisor(Function, Args...)					\
	MACRO_BLOCK_BEGIN {						\
		struct Function ## _Parameters __sv_parameters = {	\
			Args						\
		};							\
		enum FunctionIndex __sv_function = Function ## _Index;	\
									\
		supervisor(__sv_function,				\
			   &sv_parameters, sizeof __sv_parameters);	\
	} MACRO_BLOCK_END

void supervisor(enum FunctionIndex, void *, size_t);

#endif /* !_SK_SUPERVISOR_H_ */
