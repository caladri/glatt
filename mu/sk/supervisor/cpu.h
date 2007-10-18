#ifndef	_SUPERVISOR_CPU_H_
#define	_SUPERVISOR_CPU_H_


enum cpu_state {
	CPU_PRESENT,
	CPU_RUNNING,
	CPU_INSTALLED,
};

void supervisor_cpu_add(cpu_id_t, enum cpu_state);
void supervisor_cpu_add_child(cpu_id_t, cpu_id_t, enum cpu_state);
void supervisor_cpu_installed(cpu_id_t);

#endif /* !_SUPERVISOR_CPU_H_ */
