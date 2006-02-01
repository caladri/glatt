#ifndef	_CPU_CPUINFO_H_
#define	_CPU_CPUINFO_H_

struct cpuinfo {
	uint32_t cpu_ntlbs;	/* Number of TLB entries.  */
};

struct cpuinfo cpu_identify(void);

#endif /* _CPU_CPUINFO_H_ */
