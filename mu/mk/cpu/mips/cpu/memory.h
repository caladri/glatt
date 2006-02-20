#ifndef	_CPU_MEMORY_H_
#define	_CPU_MEMORY_H_

	/* 64-bit user virtual address space.  */

#define	XUSEG_BASE	(0x0000000000000000)
#define	XUSEG_END	(0x000000ffffffffff)

	/* 64-bit kernel virtual address space.  */

#define	XKSEG_BASE	(0xc000000000000000)
#define	XKSEG_END	(0xc00000ff7fffffff)
	/* We skip one page for PCPU data.  */
#define	KERNEL_BASE	(XKSEG_BASE + PAGE_SIZE)
#define	KERNEL_END	(XKSEG_END)

	/* 64-bit kernel physical address space mapping.  */

#define	XKPHYS_BASE	(0x8000000000000000)
#define	XKPHYS_END	(0xbfffffffffffffff)
#define	XKPHYS_MAP(cca,a)						\
	((void *)((0x2ULL << 62) | ((uintptr_t)(cca) << 59) | (uintptr_t)(a)))
#define	XKPHYS_EXTRACT(a)						\
	((uintptr_t)(a) & 0x0effffffffffffff)

	/* 64-bit kernel physical address space cache modes.  */

#define	XKPHYS_UC	(2)	/* Uncached.  */
#define	XKPHYS_CNC	(3)	/* Cacheable non-coherent.  */
#define	XKPHYS_CCE	(4)	/* Cacheable coherent exclusive.  */
#define	XKPHYS_CCEW	(5)	/* Cacheable coherent exclusive on write.  */
#define	XKPHYS_CCUW	(6)	/* Cacheable coherent update on write.  */

#endif /* !_CPU_MEMORY_H_ */
