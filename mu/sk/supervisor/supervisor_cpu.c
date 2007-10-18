#include <sk/types.h>
#include <sk/queue.h>
#include <supervisor/cpu.h>
#include <supervisor/memory.h>

struct cpu {
	cpu_id_t cpu_id;
	enum cpu_state cpu_state;
	STAILQ_ENTRY(struct cpu) cpu_link;
	STAILQ_ENTRY(struct cpu) cpu_peers;
	STAILQ_HEAD(, struct cpu) cpu_children;
};

static STAILQ_HEAD(, struct cpu) supervisor_cpu_queue =
	STAILQ_HEAD_INITIALIZER(supervisor_cpu_queue);

static STAILQ_HEAD(, struct cpu) supervisor_cpu_topology =
	STAILQ_HEAD_INITIALIZER(supervisor_cpu_topology);

static void supervisor_cpu_add_topology(struct cpu *, struct cpu *);
static struct cpu *supervisor_cpu_alloc(cpu_id_t, enum cpu_state);
static struct cpu *supervisor_cpu_lookup(cpu_id_t);

void
supervisor_cpu_add(cpu_id_t cpuid, enum cpu_state state)
{
	struct cpu *cpu;

	cpu = supervisor_cpu_alloc(cpuid, state);
	supervisor_cpu_add_topology(NULL, cpu);
}

void
supervisor_cpu_add_child(cpu_id_t parentid, cpu_id_t cpuid, enum cpu_state state)
{
	struct cpu *cpu, *parent;

	parent = supervisor_cpu_lookup(parentid);
	if (parent == NULL) {
		/* XXX panic */
		return;
	}
	cpu = supervisor_cpu_alloc(cpuid, state);
	supervisor_cpu_add_topology(parent, cpu);
}

void
supervisor_cpu_installed(cpu_id_t cpuid)
{
	struct cpu *cpu;

	cpu = supervisor_cpu_lookup(cpuid);
	if (cpu == NULL) {
		/* XXX panic */
		return;
	}
	switch (cpu->cpu_state) {
	case CPU_PRESENT:
		cpu->cpu_state = CPU_INSTALLED;
		break;
	default:
		/* XXX panic */
		break;
	}
}

static void
supervisor_cpu_add_topology(struct cpu *parent, struct cpu *cpu)
{
	if (parent != NULL) {
		STAILQ_INSERT_TAIL(&parent->cpu_children, cpu, cpu_peers);
	} else {
		STAILQ_INSERT_TAIL(&supervisor_cpu_topology, cpu, cpu_peers);
	}
}

static struct cpu *
supervisor_cpu_alloc(cpu_id_t id, enum cpu_state state)
{
	struct cpu *cpu;

	cpu = supervisor_memory_alloc(sizeof *cpu);
	cpu->cpu_id = id;
	cpu->cpu_state = state;
	STAILQ_INIT(&cpu->cpu_children);
	STAILQ_INSERT_TAIL(&supervisor_cpu_queue, cpu, cpu_link);
	return (cpu);
}

static struct cpu *
supervisor_cpu_lookup(cpu_id_t id)
{
	struct cpu *cpu;

	STAILQ_FOREACH(cpu, &supervisor_cpu_queue, cpu_link) {
		if (cpu->cpu_id != id)
			continue;
		return (cpu);
	}
	return (NULL);
}
