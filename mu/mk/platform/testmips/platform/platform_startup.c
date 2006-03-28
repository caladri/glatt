#include <core/types.h>
#include <core/mp.h>
#include <core/startup.h>
#if 1
#include <cpu/memory.h>
#include <db/db.h>
#include <io/device/console/console.h>
#include <vm/alloc.h>
#include <vm/page.h>
#include <vm/vm.h>
#endif

void
platform_startup(void)
{
	cpu_startup();
	platform_mp_startup();
#if 1
	int error;
	paddr_t paddr;
	vaddr_t vaddr;
	paddr_t *ep;

	error = vm_alloc(&kernel_vm, PAGE_SIZE, &vaddr);
	ASSERT(error == 0, "vm_alloc failed");
	//kcprintf("vm_alloc gave us %p\n", (void *)vaddr);
	error = page_extract(&kernel_vm, vaddr, &paddr);
	ASSERT(error == 0, "page_extract failed");
	//kcprintf("page_extract gave us %p\n", (void *)paddr);
	ep = (paddr_t *)vaddr;
	//kcprintf("casted.\n");
	*ep = paddr;
	//kcprintf("assigned.\n");
	ep = (paddr_t *)XKPHYS_MAP(XKPHYS_UC, paddr);
	//kcprintf("Checking %p\n", (void *)ep);
	ASSERT(*ep == paddr, "mismatch!");
	vm_free(&kernel_vm, PAGE_SIZE, vaddr);
#endif
	startup_main();
}
