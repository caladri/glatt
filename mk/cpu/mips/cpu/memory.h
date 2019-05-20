#ifndef	_CPU_MEMORY_H_
#define	_CPU_MEMORY_H_

#ifdef ASSEMBLER
#define	ADDRESS_C(a)	(a)
#else
#define	ADDRESS_C(a)	(a ## ull)
#endif

	/* 64-bit user virtual address space.  */

#define	XUSEG_BASE	ADDRESS_C(0x0000000000000000)
#define	XUSEG_END	ADDRESS_C(0x000000ffffffffff)

	/* Address space to use for userland.  */

#define	USER_BASE	(XUSEG_BASE)
#define	USER_END	(XUSEG_END)

	/* 64-bit direct-mapped kernel address space.  */

#define	XKPHYS_BASE	ADDRESS_C(0x8000000000000000)
#define	XKPHYS_END	ADDRESS_C(0xbfffffffffffffff)

	/* 64-bit kernel virtual address space.  */

#define	XKSEG_BASE	ADDRESS_C(0xc000000000000000)
#define	XKSEG_END	ADDRESS_C(0xc00000ff7fffffff)

	/* 32-bit cached direct-mapped kernel address space.  */

#define	KSEG0_BASE	ADDRESS_C(0xffffffff80000000)
#define	KSEG0_END	ADDRESS_C(0xffffffff9fffffff)

	/* 32-bit uncached direct-mapped kernel address space.  */

#define	KSEG1_BASE	ADDRESS_C(0xffffffffa0000000)
#define	KSEG1_END	ADDRESS_C(0xffffffffbfffffff)

	/* 32-bit kernel virtual address space.  */

#define	KSEG2_BASE	ADDRESS_C(0xffffffffc0000000)
#define	KSEG2_END	ADDRESS_C(0xffffffffffffffff)

	/* Address space to use for the kernel.  */

#define	KERNEL_BASE	(XKSEG_BASE)
#define	KERNEL_END	(XKSEG_END)

	/* 32-bit kernel physical address space mapping.  */

#define	KSEG0_MAP(a)	((void *)(KSEG0_BASE | (uintptr_t)(a)))
#define	KSEG1_MAP(a)	((void *)(KSEG1_BASE | (uintptr_t)(a)))
#define	KSEG_EXTRACT(a)	((uintptr_t)(a) & 0x1fffffff)

	/* 64-bit kernel physical address space mapping.  */

#define	XKPHYS_MAP(cca,a)						\
	((void *)((0x2ULL << 62) | ((uintptr_t)(cca) << 59) | (uintptr_t)(a)))
#define	XKPHYS_EXTRACT(a)						\
	((uintptr_t)(a) & 0x07ffffffffffffff)

	/* Cache coherency attributes.  */

#define	CCA_UC		(0x02)	/* Uncached.  */
#define	CCA_CNC		(0x03)	/* Cacheable non-coherent.  */
#define	CCA_MASK	(0x07)

#endif /* !_CPU_MEMORY_H_ */
