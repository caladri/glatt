#ifndef	_CPU_CPUINFO_H_
#define	_CPU_CPUINFO_H_

struct cpuinfo {
	const char *cpu_company;	/* Manufacturer.  */
	const char *cpu_type;		/* CPU type.  */
	uint32_t cpu_ntlbs;		/* Number of TLB entries.  */
	uint8_t cpu_revision_major;	/* Revision major number.  */
	uint8_t cpu_revision_minor;	/* Revision minor number.  */
	bool cpu_mips3264isa;		/* MIPS32/64 ISA.  */
};

void cpu_identify(struct cpuinfo *) __non_null(1);

#endif /* !_CPU_CPUINFO_H_ */
