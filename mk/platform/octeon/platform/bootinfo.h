#ifndef	PLATFORM_BOOTINFO_H
#define	PLATFORM_BOOTINFO_H

struct octeon_boot_descriptor {
	uint32_t version;
	uint32_t size;
	uint8_t unused1[52];
	uint32_t argc;
	uint32_t argv[64];
	uint8_t unused2[67];
	uint64_t info;
};

struct octeon_boot_info {
	uint32_t major;
	uint32_t minor;
	uint8_t unused1[44];
	uint32_t cores;
	uint32_t dram_mb;
	uint32_t memory_descriptor;
	uint8_t unused2[4];
	uint32_t clock_hz;
	uint32_t dram_hz;
	uint8_t unused3[4];
	uint16_t board;
	uint8_t board_major;
	uint8_t board_minor;
	uint8_t unused4[4];
	char serial_number[20];
	uint8_t mac_base[6];
	uint8_t mac_count;
};

struct octeon_boot_memory_descriptor {
	uint8_t unused1[8];
	uint64_t head;
	uint32_t major;
	uint32_t minor;
	uint8_t unused2[32];
};

struct octeon_memory_descriptor {
	uint64_t next;
	uint64_t size;
};

#endif /* !PLATFORM_BOOTINFO_H */
