#ifndef	_SUPERVISOR_MEMORY_H_
#define	_SUPERVISOR_MEMORY_H_

enum supervisor_memory_type {
	MEMORY_GLOBAL,
	MEMORY_LOCAL,
};

void *supervisor_memory_direct_map(paddr_t);
void supervisor_memory_insert(paddr_t, size_t, enum supervisor_memory_type);

#endif /* !_SUPERVISOR_MEMORY_H_ */
