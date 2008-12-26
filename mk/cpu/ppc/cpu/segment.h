#ifndef	_CPU_SEGMENT_H_
#define	_CPU_SEGMENT_H_

#define	SEGMENT_SHIFT	(28)
#define	SEGMENT_AMASK	((1 << SEGMENT_SHIFT) - 1)
#define	SEGMENT_SMASK	(0xf << SEGMENT_SHIFT)
#define	SEGMENT_SIZE	(0x10000000)

#define	SEGMENT_BASE(seg)						\
	((vaddr_t)((seg) << SEGMENT_SHIFT))

#define	SEGMENT_END(seg)						\
	(SEGMENT_BASE((seg)) + SEGMENT_SIZE)

#define	SEGMENT_MAP(seg, addr)						\
	(SEGMENT_BASE((seg)) | ((addr) & SEGMENT_AMASK))

#define	SEGMENT_AEXTRACT(addr)						\
	((addr) & SEGMENT_AMASK)

#define	SEGMENT_SEXTRACT(addr)						\
	(((addr) & SEGMENT_SMASK) >> SEGMENT_SHIFT)

#define	KERNEL_BASE	SEGMENT_BASE(0)
#define	KERNEL_END	SEGMENT_END(0)

#define	USER_BASE	SEGMENT_BASE(1)
#define	USER_END	SEGMENT_END(1)

#endif /* !_CPU_SEGMENT_H_ */
