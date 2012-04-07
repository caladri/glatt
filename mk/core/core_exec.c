#include <core/types.h>
#include <core/error.h>
#include <core/elf64.h>
#include <core/exec.h>
#include <core/pool.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <io/console/console.h>
#include <vm/vm.h>
#include <vm/vm_alloc.h>
#include <vm/vm_index.h>
#include <vm/vm_page.h>

static int exec_elf64_load(struct thread *, exec_read_t *, void *);
static int exec_read(exec_read_t *, void *, void *, off_t, size_t);

int
exec_load(struct task *task, const char *name, exec_read_t *readf, void *softc)
{
	struct thread *td;
	int error;

	error = thread_create(&td, task, name, THREAD_DEFAULT);
	if (error != 0) {
		kcprintf("%s: thread_create for %s failed: %m\n", __func__, name, error);
		return (error);
	}

	error = exec_elf64_load(td, readf, softc);
	if (error != 0) {
		kcprintf("%s: exec_elf64_load for %s failed: %m\n", __func__, name, error);
		return (error);
	}

	scheduler_thread_runnable(td);

	return (0);
}

int
exec_task(const char *name, exec_read_t *readf, void *softc)
{
	struct task *task;
	int error;

	error = task_create(&task, name, TASK_DEFAULT);
	if (error != 0)
		return (error);

	error = exec_load(task, name, readf, softc);
	if (error != 0)
		return (error);

	return (0);
}

static int
exec_elf64_load(struct thread *td, exec_read_t *readf, void *softc)
{
	struct elf64_program_header ph;
	struct elf64_header eh;
	vaddr_t begin, end;
	vaddr_t low, high;
	vaddr_t kvaddr;
	size_t len;
	unsigned i;
	int error;

	error = exec_read(readf, softc, &eh, 0, sizeof eh);
	if (error != 0)
		return (error);

	/*
	 * Check ELF header magic and version.
	 */
	if (!ELF_HEADER_CHECK_MAGIC(&eh)) {
		kcprintf("%s: bad magic.\n", __func__);
		return (ERROR_INVALID);
	}

	if (eh.eh_version != ELF_VERSION_1) {
		kcprintf("%s: wrong version.\n", __func__);
		return (ERROR_INVALID);
	}

	/*
	 * Verify that this is 64-bit MSB ELF for big-endian MIPS.
	 *
	 * XXX Machine-independentize.
	 */
	if (eh.eh_ident[ELF_HEADER_IDENT_CLASS] != ELF_HEADER_CLASS_64) {
		kcprintf("%s: wrong ident class.\n", __func__);
		return (ERROR_INVALID);
	}

	if (eh.eh_ident[ELF_HEADER_IDENT_DATA] != ELF_HEADER_DATA_2MSB) {
		kcprintf("%s: wrong ident data format.\n", __func__);
		return (ERROR_INVALID);
	}

	if (eh.eh_machine != ELF_MACHINE_MIPSBE) {
		kcprintf("%s: wrong machine type.\n", __func__);
		return (ERROR_INVALID);
	}

	/*
	 * Verify that this is an executable with an entry point.
	 */
	if (eh.eh_type != ELF_TYPE_EXEC) {
		kcprintf("%s: not an executable.\n", __func__);
		return (ERROR_INVALID);
	}

	if (eh.eh_entry == ELF_ENTRY_NONE) {
		kcprintf("%s: no entry point defined.\n", __func__);
		return (ERROR_INVALID);
	}

	/*
	 * Check program headers.
	 */
	if (eh.eh_phentsize != sizeof ph) {
		kcprintf("%s: program headers have unexpected size.\n", __func__);
		return (ERROR_INVALID);
	}

	if (eh.eh_phnum == 0) {
		kcprintf("%s: executable has no program headers.\n", __func__);
		return (ERROR_INVALID);
	}

	/*
	 * Figure out the span of addresses the program occupies.
	 *
	 * XXX Could be better.
	 */
	low = 0;
	high = 0;
	for (i = 0; i < eh.eh_phnum; i++) {
		error = exec_read(readf, softc, &ph,
				  eh.eh_phoff + i * sizeof ph, sizeof ph);
		if (error != 0)
			panic("%s: exec_read of program header failed: %m", __func__, error);

		if (ph.ph_type != ELF_PROGRAM_HEADER_TYPE_LOAD)
			continue;

		begin = PAGE_FLOOR(ROUNDDOWN(ph.ph_vaddr, ph.ph_align));
		end = PAGE_ROUNDUP(ROUNDUP(ph.ph_vaddr + ph.ph_memorysize, ph.ph_align));

		if (high == 0) {
			low = begin;
			high = end;
			continue;
		}
		if (low > begin)
			low = begin;
		if (high < end)
			high = end;
	}

	if (high == 0 || low == high) {
		kcprintf("%s: executable has no loadable program data.\n", __func__);
		return (ERROR_INVALID);
	}

	/*
	 * Wire program data address range into the kernel for program load.
	 */
	error = vm_alloc_range_wire(td->td_task->t_vm, low, high, &kvaddr);
	if (error != 0) {
		kcprintf("%s: could not allocate requested program address range: %m", __func__, error);
		return (error);
	}

	/*
	 * Load program headers.
	 */
	for (i = 0; i < eh.eh_phnum; i++) {
		error = exec_read(readf, softc, &ph,
				  eh.eh_phoff + i * sizeof ph, sizeof ph);
		if (error != 0)
			panic("%s: exec_read of program header failed: %m", __func__, error);

		if (ph.ph_type != ELF_PROGRAM_HEADER_TYPE_LOAD)
			continue;

		len = MIN(ph.ph_memorysize, ph.ph_filesize);
		begin = kvaddr + (ph.ph_vaddr - low);
		error = exec_read(readf, softc, (void *)begin, ph.ph_off, len);
		if (error != 0)
			panic("%s: exec_read of program data failed: %m", __func__, error);
	}

	/*
	 * Unwire program data.
	 */
	error = vm_free_address(&kernel_vm, kvaddr);
	if (error != 0)
		panic("%s: could not unwire progam data: %m", __func__, error);

	/*
	 * Set up userland trampoline.
	 */
	thread_set_upcall(td, cpu_thread_user_trampoline, (void *)(uintptr_t)eh.eh_entry);

	return (0);
}

static int
exec_read(exec_read_t *readf, void *softc, void *buf, off_t off, size_t len)
{
	int error;
	size_t resid;

	resid = len;

	for (;;) {
		error = readf(softc, buf, off, &len);
		if (error != 0)
			return (error);
		ASSERT(len <= resid, ("cannot have read too much data"));
		if (len == resid)
			return (0);
		buf = (void *)((uintptr_t)buf + len);
		resid -= len;
		len = resid;
	}
	return (0);
}
