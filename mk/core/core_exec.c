#include <core/types.h>
#include <core/error.h>
#include <core/elf64.h>
#include <core/exec.h>
#include <core/pool.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <fs/fs_ops.h>
#include <core/console.h>
#include <vm/vm.h>
#include <vm/vm_alloc.h>
#include <vm/vm_index.h>
#include <vm/vm_page.h>

static int exec_elf64_load(struct vm *, vaddr_t *, fs_file_read_op_t *, fs_context_t, fs_file_context_t);
static int exec_read(fs_file_read_op_t *, fs_context_t, fs_file_context_t, void *, off_t, size_t);

int
exec_load(struct vm *vm, vaddr_t *entryp, const char *name, fs_file_read_op_t *readf, fs_context_t fsc, fs_file_context_t fsfc)
{
	int error;

	error = exec_elf64_load(vm, entryp, readf, fsc, fsfc);
	if (error != 0) {
		printf("%s: exec_elf64_load for %s failed: %m\n", __func__, name, error);
		return (error);
	}

	return (0);
}

int
exec_task(ipc_port_t parent, ipc_port_t *childp, const char *name, fs_file_read_op_t *readf, fs_context_t fsc, fs_file_context_t fsfc)
{
	struct thread *td;
	struct task *task;
	vaddr_t entry;
	int error;

	error = task_create(parent, &task, name, TASK_DEFAULT);
	if (error != 0)
		return (error);

	if (childp != NULL)
		*childp = task->t_ipc.ipct_task_port;

	error = thread_create(&td, task, name, THREAD_DEFAULT | THREAD_USTACK);
	if (error != 0) {
		printf("%s: thread_create for %s failed: %m\n", __func__, name, error);
		return (error);
	}

	error = exec_load(task->t_vm, &entry, name, readf, fsc, fsfc);
	if (error != 0)
		return (error);

	/*
	 * Set up userland entry-point.
	 */
	thread_set_upcall_user(td, entry, 0);

	scheduler_thread_runnable(td);

	return (0);
}

static int
exec_elf64_load(struct vm *vm, vaddr_t *entryp, fs_file_read_op_t *readf, fs_context_t fsc, fs_file_context_t fsfc)
{
	struct elf64_program_header ph;
	struct elf64_header eh;
	vaddr_t begin, end;
	vaddr_t low, high;
	vaddr_t kvaddr;
	size_t len, o;
	unsigned i;
	int error;

	error = exec_read(readf, fsc, fsfc, &eh, 0, sizeof eh);
	if (error != 0)
		return (error);

	/*
	 * Check ELF header magic and version.
	 */
	if (!ELF_HEADER_CHECK_MAGIC(&eh)) {
		printf("%s: bad magic.\n", __func__);
		return (ERROR_INVALID);
	}

	if (eh.eh_version != ELF_VERSION_1) {
		printf("%s: wrong version.\n", __func__);
		return (ERROR_INVALID);
	}

	/*
	 * Verify that this is 64-bit MSB ELF for big-endian MIPS.
	 *
	 * XXX Machine-independentize.
	 */
	if (eh.eh_ident[ELF_HEADER_IDENT_CLASS] != ELF_HEADER_CLASS_64) {
		printf("%s: wrong ident class.\n", __func__);
		return (ERROR_INVALID);
	}

	if (eh.eh_ident[ELF_HEADER_IDENT_DATA] != ELF_HEADER_DATA_2MSB) {
		printf("%s: wrong ident data format.\n", __func__);
		return (ERROR_INVALID);
	}

	if (eh.eh_machine != ELF_MACHINE_MIPSBE) {
		printf("%s: wrong machine type.\n", __func__);
		return (ERROR_INVALID);
	}

	/*
	 * Verify that this is an executable with an entry point.
	 */
	if (eh.eh_type != ELF_TYPE_EXEC) {
		printf("%s: not an executable.\n", __func__);
		return (ERROR_INVALID);
	}

	if (eh.eh_entry == ELF_ENTRY_NONE) {
		printf("%s: no entry point defined.\n", __func__);
		return (ERROR_INVALID);
	}

	/*
	 * Check program headers.
	 */
	if (eh.eh_phentsize != sizeof ph) {
		printf("%s: program headers have unexpected size.\n", __func__);
		return (ERROR_INVALID);
	}

	if (eh.eh_phnum == 0) {
		printf("%s: executable has no program headers.\n", __func__);
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
		error = exec_read(readf, fsc, fsfc, &ph,
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
		printf("%s: executable has no loadable program data.\n", __func__);
		return (ERROR_INVALID);
	}

	/*
	 * Wire program data address range into the kernel for program load.
	 */
	error = vm_alloc_range_wire(vm, low, high, &kvaddr, &o);
	if (error != 0) {
		printf("%s: could not allocate requested program address range: %m\n", __func__, error);
		return (error);
	}
	ASSERT(o == 0, ("cannot have offset into what should be an aligned page."));

	/*
	 * Zero everything.
	 */
	memset((void *)kvaddr, 0, high - low);

	/*
	 * Load program headers.
	 */
	for (i = 0; i < eh.eh_phnum; i++) {
		error = exec_read(readf, fsc, fsfc, &ph,
				  eh.eh_phoff + i * sizeof ph, sizeof ph);
		if (error != 0)
			panic("%s: exec_read of program header failed: %m", __func__, error);

		if (ph.ph_type != ELF_PROGRAM_HEADER_TYPE_LOAD)
			continue;

		/* XXX Where memorysize is > filesize do we need to zero?  */
		len = MIN(ph.ph_memorysize, ph.ph_filesize);
		begin = kvaddr + (ph.ph_vaddr - low);
		error = exec_read(readf, fsc, fsfc, (void *)begin, ph.ph_off, len);
		if (error != 0)
			panic("%s: exec_read of program data failed: %m", __func__, error);
	}

	/*
	 * Unwire program data.
	 */
	error = vm_unwire(vm, low, high - low, kvaddr);
	if (error != 0)
		panic("%s: could not unwire progam data: %m", __func__, error);

	*entryp = (vaddr_t)eh.eh_entry;

	return (0);
}

static int
exec_read(fs_file_read_op_t *readf, fs_context_t fsc, fs_file_context_t fsfc, void *buf, off_t off, size_t len)
{
	int error;
	size_t resid;

	resid = len;

	for (;;) {
		error = readf(fsc, fsfc, buf, off, &len);
		if (error != 0)
			return (error);
		ASSERT(len <= resid, ("cannot have read too much data"));
		if (len == resid)
			return (0);
		buf = (void *)((uintptr_t)buf + len);
		off += len;
		resid -= len;
		len = resid;
	}
	return (0);
}
