#ifndef	_CORE_ELF64_H_
#define	_CORE_ELF64_H_

/*
 * This is for the ELF binary loader for Glatt MU/MK.  It is intended to load
 * straightforward static-linked binaries for the purpose of launching servers
 * and the dynamic loader.  Servers and the dynamic loader are expected to be
 * linked at an address that allows them to load user binaries in their address
 * space without conflicts.
 *
 * Information here gleaned from a variety of sources.  The authoritative one
 * is: http://www.sco.com/developers/gabi/latest/contents.html
 *
 * XXX Explicitly just supporting ELF64 for now, but that will surely change.
 */

#define	ELF_HEADER_IDENT_LENGTH	(16)

#define	ELF_HEADER_MAGIC	"\177ELF"
#define	ELF_HEADER_CHECK_MAGIC(ehdr)					\
	((ehdr)->eh_ident[0] == ELF_HEADER_MAGIC[0] &&			\
	 (ehdr)->eh_ident[1] == ELF_HEADER_MAGIC[1] &&			\
	 (ehdr)->eh_ident[2] == ELF_HEADER_MAGIC[2] &&			\
	 (ehdr)->eh_ident[3] == ELF_HEADER_MAGIC[3])

#define	ELF_HEADER_IDENT_CLASS	(4)
#define	ELF_HEADER_CLASS_64	(0x02)

#define	ELF_HEADER_IDENT_DATA	(5)
#define	ELF_HEADER_DATA_2MSB	(0x02)

#define	ELF_TYPE_EXEC		(0x0002)

#define	ELF_MACHINE_MIPSBE	(0x0008)
#define	ELF_MACHINE_MIPSLE	(0x000a)

#define	ELF_VERSION_1		(0x0001)

#define	ELF_ENTRY_NONE		(0)

#define	ELF_PROGRAM_HEADER_TYPE_LOAD	(1)

#define	ELF_PROGRAM_HEADER_FLAGS_EXEC	(0x00000001)
#define	ELF_PROGRAM_HEADER_FLAGS_WRITE	(0x00000004)
#define	ELF_PROGRAM_HEADER_FLAGS_READ	(0x00000004)


struct elf64_header {
	unsigned char eh_ident[ELF_HEADER_IDENT_LENGTH];
	uint16_t eh_type;
	uint16_t eh_machine;
	uint32_t eh_version;
	uint64_t eh_entry;
	uint64_t eh_phoff;
	uint64_t eh_shoff;
	uint32_t eh_flags;
	uint16_t eh_ehsize;
	uint16_t eh_phentsize;
	uint16_t eh_phnum;
	uint16_t eh_shentsize;
	uint16_t eh_shnum;
	uint16_t eh_shstrndx;
};

struct elf64_program_header {
	uint32_t ph_type;
	uint32_t ph_flags;
	uint64_t ph_off;
	uint64_t ph_vaddr;
	uint64_t ph_paddr;
	uint64_t ph_filesize;
	uint64_t ph_memorysize;
	uint64_t ph_align;
};

#endif /* !_CORE_ELF64_H_ */
