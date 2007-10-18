#ifndef	_SUPERVISOR_CPU_H_
#define	_SUPERVISOR_CPU_H_

void supervisor_cpu_add(cpu_id_t);
void supervisor_cpu_add_child(cpu_id_t, cpu_id_t);
void supervisor_cpu_installed(cpu_id_t);

#endif /* !_SUPERVISOR_CPU_H_ */
