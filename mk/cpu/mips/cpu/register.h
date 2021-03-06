#ifndef	_CPU_REGISTER_H_
#define	_CPU_REGISTER_H_

#ifdef ASSEMBLER
#define	_REGNUM(r)	$r
#else
#define	_REGNUM(r)	r
#endif

#ifndef	ASSEMBLER
typedef	uint64_t	register_t;
#endif

	/* General-purpose registers.  */

#ifdef ASSEMBLER
#define	zero	_REGNUM(0)
#define	AT	_REGNUM(at)
#define	v0	_REGNUM(2)
#define	v1	_REGNUM(3)
#define	a0	_REGNUM(4)
#define	a1	_REGNUM(5)
#define	a2	_REGNUM(6)
#define	a3	_REGNUM(7)
#define	a4	_REGNUM(8)
#define	a5	_REGNUM(9)
#define	a6	_REGNUM(10)
#define	a7	_REGNUM(11)
#define	t0	_REGNUM(12)
#define	t1	_REGNUM(13)
#define	t2	_REGNUM(14)
#define	t3	_REGNUM(15)
#define	s0	_REGNUM(16)
#define	s1	_REGNUM(17)
#define	s2	_REGNUM(18)
#define	s3	_REGNUM(19)
#define	s4	_REGNUM(20)
#define	s5	_REGNUM(21)
#define	s6	_REGNUM(22)
#define	s7	_REGNUM(23)
#define	t8	_REGNUM(24)
#define	t9	_REGNUM(25)
#define	k0	_REGNUM(26)
#define	k1	_REGNUM(27)
#define	gp	_REGNUM(28)
#define	sp	_REGNUM(29)
#define	s8	_REGNUM(30)
#define	ra	_REGNUM(31)
#endif

	/* Coprocessor 0 registers.  */

#define	CP0_TLBINDEX	_REGNUM(0)
#define	CP0_TLBRANDOM	_REGNUM(1)
#define	CP0_TLBENTRYLO0	_REGNUM(2)
#define	CP0_TLBENTRYLO1	_REGNUM(3)
#define	CP0_TLBCONTEXT	_REGNUM(4)
#define	CP0_TLBPAGEMASK	_REGNUM(5)
#define	CP0_TLBWIRED	_REGNUM(6)
#define	CP0_BADVADDR	_REGNUM(8)
#define	CP0_COUNT	_REGNUM(9)
#define	CP0_TLBENTRYHI	_REGNUM(10)
#define	CP0_COMPARE	_REGNUM(11)
#define	CP0_STATUS	_REGNUM(12)
#define	CP0_CAUSE	_REGNUM(13)
#define	CP0_EXCPC	_REGNUM(14)
#define	CP0_PRID	_REGNUM(15)
#define	CP0_CONFIG	_REGNUM(16)
#define	CP0_LLADDR	_REGNUM(17)
#define	CP0_WATCHLO	_REGNUM(18)
#define	CP0_WATCHHI	_REGNUM(19)
#define	CP0_TLBXCONTEXT	_REGNUM(20)
#define	CP0_ECC		_REGNUM(26)
#define	CP0_CACHEERR	_REGNUM(27)
#define	CP0_TAGLO	_REGNUM(28)
#define	CP0_TAGHI	_REGNUM(29)
#define	CP0_ERRORPC	_REGNUM(30)

	/* Coprocessor 0 cause register bits & shifts.  */
#define	CP0_CAUSE_EXCEPTION		(0x7c)
#define	CP0_CAUSE_EXCEPTION_SHIFT	(2)

#define	CP0_CAUSE_INTERRUPT_SHIFT	(8)
#define	CP0_CAUSE_INTERRUPT_MASK	(0xff00)

	/* Coprocessor 0 status register bits & shifts.  */

#define	CP0_STATUS_IE	0x00000001	/* Interrupts enabled.  */
#define	CP0_STATUS_EXL	0x00000002	/* Error level.  */
#define	CP0_STATUS_ERL	0x00000004	/* Exception level.  */
#define	CP0_STATUS_U	0x00000010	/* User privileges.  */
#define	CP0_STATUS_UX	0x00000020	/* User extended mode.  */
#define	CP0_STATUS_SX	0x00000040	/* Supervisor extended mode.  */
#define	CP0_STATUS_KX	0x00000080	/* Kernel extended mode.  */
#define	CP0_STATUS_SR	0x00100000	/* Soft reset.  */
#define	CP0_STATUS_BEV	0x00400000	/* Using boot exception vectors.  */
#define	CP0_STATUS_PX	0x00800000	/* Allow 64-bit register access.  */

#define	CP0_STATUS_INTERRUPT_SHIFT	(8)
#define	CP0_STATUS_INTERRUPT_MASK	(0xff00)

	/* Coprocessor 0 status register states.  */
#define	KERNEL_STATUS	(CP0_STATUS_UX | CP0_STATUS_KX | CP0_STATUS_PX)
#define	USER_STATUS	(CP0_STATUS_U | CP0_STATUS_UX | CP0_STATUS_KX | CP0_STATUS_PX | CP0_STATUS_IE)

	/* Coprocessor 0 config register selector 1 bits & shifts.  */

#define	CP0_CONFIG1_MMUSIZE_SHIFT	(25)
#define	CP0_CONFIG1_MMUSIZE_MASK	(0x3f << CP0_CONFIG1_MMUSIZE_SHIFT)

#define	CP0_CONFIG1_ICACHE_LSIZE_SHIFT	(19)
#define	CP0_CONFIG1_ICACHE_LSIZE_MASK	(0x03 << CP0_CONFIG1_ICACHE_LSIZE_SHIFT)

#define	CP0_CONFIG1_DCACHE_LSIZE_SHIFT	(10)
#define	CP0_CONFIG1_DCACHE_LSIZE_MASK	(0x03 << CP0_CONFIG1_DCACHE_LSIZE_SHIFT)

#endif /* !_CPU_REGISTER_H_ */
