#ifndef	_CORE_ENDIAN_H_
#define	_CORE_ENDIAN_H_

static inline uint16_t
bswap16(const uint16_t x)
{
	return (((x & 0xff00u) >> 0x08) |
		((x & 0x00ffu) << 0x08));
}

static inline uint32_t
bswap32(const uint32_t x)
{
	return (((x & 0xff000000u) >> 0x18) |
		((x & 0x00ff0000u) >> 0x08) |
		((x & 0x0000ff00u) << 0x08) |
		((x & 0x000000ffu) << 0x18));
}

static inline uint64_t
bswap64(const uint64_t x)
{
	return (((x & 0xff00000000000000ull) >> 0x38) |
		((x & 0x00ff000000000000ull) >> 0x28) |
		((x & 0x0000ff0000000000ull) >> 0x18) |
		((x & 0x000000ff00000000ull) >> 0x08) |
		((x & 0x00000000ff000000ull) << 0x08) |
		((x & 0x0000000000ff0000ull) << 0x18) |
		((x & 0x000000000000ff00ull) << 0x28) |
		((x & 0x00000000000000ffull) << 0x38));
}

#endif /* !_CORE_ENDIAN_H_ */
