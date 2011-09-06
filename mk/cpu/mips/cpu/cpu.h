#ifndef	_CPU_CPU_H_
#define	_CPU_CPU_H_

#include <cpu/register.h>
#ifdef DB
#include <db/db_command.h>
#endif

static inline void
cpu_barrier(void)
{
	asm volatile ("" : : : "memory");
}

	/* Coprocessor 0 manipulation inlines for 64-bit registers.  */

#define	CP0_RW64(name, number)						\
static inline uint64_t							\
cpu_read_ ## name(void)							\
{									\
	uint64_t result;						\
	asm volatile ("dmfc0 %[result], $" STRING(number) "\n"		\
		      : [result] "=&r"(result));			\
	cpu_barrier();							\
	return (result);						\
}									\
									\
static inline void							\
cpu_write_ ## name(uint64_t value)					\
{									\
	asm volatile ("dmtc0 %[value], $" STRING(number) "\n"		\
		      : : [value] "r"(value));				\
	cpu_barrier();							\
}									\
struct __hack

CP0_RW64(badvaddr, CP0_BADVADDR);
CP0_RW64(tlb_entryhi, CP0_TLBENTRYHI);
CP0_RW64(tlb_entrylo0, CP0_TLBENTRYLO0);
CP0_RW64(tlb_entrylo1, CP0_TLBENTRYLO1);
CP0_RW64(tlb_pagemask, CP0_TLBPAGEMASK);
CP0_RW64(count, CP0_COUNT);

#undef CP0_RW64

	/* Coprocessor 0 manipulation inlines for 32-bit registers.  */

#define	CP0_RW32(name, number)						\
static inline uint32_t							\
cpu_read_ ## name(void)							\
{									\
	uint32_t result;						\
	asm volatile ("mfc0 %[result], $" STRING(number) "\n"		\
		      : [result] "=&r"(result));			\
	cpu_barrier();							\
	return (result);						\
}									\
									\
static inline void							\
cpu_write_ ## name(uint32_t value)					\
{									\
	asm volatile ("mtc0 %[value], $" STRING(number) "\n"		\
		      : : [value] "r"(value));				\
	cpu_barrier();							\
}									\
struct __hack

CP0_RW32(config, CP0_CONFIG);
CP0_RW32(cause, CP0_CAUSE);
CP0_RW32(status, CP0_STATUS);
CP0_RW32(compare, CP0_COMPARE);
CP0_RW32(tlb_index, CP0_TLBINDEX);
CP0_RW32(tlb_wired, CP0_TLBWIRED);
CP0_RW32(prid, CP0_PRID);

#undef CP0_RW32

#if __mips == 64
#define	CP0_S_RW32(name, number, s)					\
static inline uint32_t							\
cpu_read_ ## name ## s(void)						\
{									\
	uint32_t result;						\
	asm volatile ("mfc0 %[result], $" STRING(number)		\
		      ", " STRING(s) "\n"				\
		      : [result] "=&r"(result));			\
	cpu_barrier();							\
	return (result);						\
}									\
									\
static inline void							\
cpu_write_ ## name ## s(uint32_t value)					\
{									\
	asm volatile ("mtc0 %[value], $" STRING(number)			\
		      ", " STRING(s) "\n"				\
		      : : [value] "r"(value));				\
	cpu_barrier();							\
}									\
struct __hack

CP0_S_RW32(config, CP0_CONFIG, 1);

#undef CP0_RW32
#endif

#ifdef DB
DB_COMMAND_TREE_DECLARE(cpu);
#endif

#endif /* !_CPU_CPU_H_ */
