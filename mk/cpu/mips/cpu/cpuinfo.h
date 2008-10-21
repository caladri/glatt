#ifndef	_CPU_CPUINFO_H_
#define	_CPU_CPUINFO_H_

struct cpuinfo {
	const char *cpu_company;	/* Manufacturer.  */
	const char *cpu_type;		/* CPU type.  */
	uint32_t cpu_ntlbs;		/* Number of TLB entries.  */
	uint8_t cpu_revision_major;	/* Revision major number.  */
	uint8_t cpu_revision_minor;	/* Revision minor number.  */
};

struct cpuinfo cpu_identify(void);

#endif /* _CPU_CPUINFO_H_ */
