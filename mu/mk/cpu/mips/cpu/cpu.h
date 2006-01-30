#ifndef	_CPU_CPU_H_
#define	_CPU_CPU_H_

#ifdef ASSEMBLER
#error "Cannot use <cpu/cpu.h> from asm."
#endif

#include <core/macro.h>
#include <cpu/register.h>

static inline void
cpu_barrier(void)
{
	__asm __volatile (".set noreorder\n\t"
			  "nop\n\t"
			  "nop\n\t"
			  "nop\n\t"
			  "nop\n\t"
			  "nop\n\t"
			  "nop\n\t"
			  "nop\n\t"
			  "nop\n\t"
			  ".set reorder\n\t"
			  : : : "memory");
}

	/* Coprocessor 0 manipulation inlines for 64-bit registers.  */

#define	CP0_RW64(name, number)						\
static inline uint64_t							\
cpu_read_ ## name(void)							\
{									\
	uint64_t result;						\
	__asm __volatile ("dmfc0 %[result], $" STRING(number) "\n"	\
		      : [result] "=&r"(result));			\
	cpu_barrier();							\
	return (result);						\
}									\
									\
static inline void							\
cpu_write_ ## name(uint64_t value)					\
{									\
	__asm __volatile ("dmtc0 %[value], $" STRING(number) "\n"	\
		      : : [value] "r"(value));				\
	cpu_barrier();							\
}									\
struct __hack

CP0_RW64(entrylo0, CP0_ENTRYLO0);
CP0_RW64(entrylo1, CP0_ENTRYLO1);
CP0_RW64(entryhi, CP0_ENTRYHI);
CP0_RW64(pagemask, CP0_PAGEMASK);
CP0_RW64(xcontext, CP0_XCONTEXT);

#undef CP0_RW64

	/* Coprocessor 0 manipulation inlines for 32-bit registers.  */

#define	CP0_RW32(name, number)						\
static inline uint32_t							\
cpu_read_ ## name(void)							\
{									\
	uint32_t result;						\
	__asm __volatile ("mfc0 %[result], $" STRING(number) "\n"	\
		      : [result] "=&r"(result));			\
	cpu_barrier();							\
	return (result);						\
}									\
									\
static inline void							\
cpu_write_ ## name(uint32_t value)					\
{									\
	__asm __volatile ("mtc0 %[value], $" STRING(number) "\n"	\
		      : : [value] "r"(value));				\
	cpu_barrier();							\
}									\
struct __hack

CP0_RW32(config, CP0_CONFIG);
CP0_RW32(cause, CP0_CAUSE);
CP0_RW32(status, CP0_STATUS);
CP0_RW32(prid, CP0_PRID);

#undef CP0_RW32

#endif /* !_CPU_CPU_H_ */
