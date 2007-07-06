#ifndef	_CPU_SK_MEMORY_H_
#define	_CPU_SK_MEMORY_H_

/*
 * Extended (64-bit) virtual->physical direct-mapped region.
 */
#define	XKPHYS_CACHE_UC		(0x2)
#define	XKPHYS_CACHE_CNC	(0x3)
#define	XKPHYS_CACHE_CCE	(0x4)
#define	XKPHYS_CACHE_CCEW	(0x5)
#define	XKPHYS_CACHE_CCUW	(0x6)

#define	XKPHYS_BASE_C(cache)						\
	((0x2ULL << 62) | ((uintptr_t)(cache) << 59))
#define	XKPHYS_MAP_C(cache, addr)					\
	((void *)(XKPHYS_BASE_C(cache) | ((uintptr_t)(addr))))
#define	XKPHYS_MAP_DEV(addr)	XKPHYS_MAP_C(XKPHYS_CACHE_UC, (addr))
#define	XKPHYS_MAP_RAM(addr)	XKPHYS_MAP_C(XKPHYS_CACHE_CCEW, (addr))

#endif /* !_CPU_SK_MEMORY_H_ */
