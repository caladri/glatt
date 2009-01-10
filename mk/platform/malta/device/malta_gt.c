#include <core/types.h>
#include <core/critical.h>
#include <core/error.h>
#include <cpu/cpu.h>
#include <cpu/cpuinfo.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>
#include <io/device/bus.h>
#include <io/pci/pci.h>
#include <io/pci/pcireg.h>

#define	GT_ADDR32(base, offset)						\
	(volatile uint32_t *)XKPHYS_MAP(XKPHYS_UC, (base) | (offset))
#define	GT_READ32(base, offset)						\
	(volatile uint32_t)*GT_ADDR32((base), (offset))
#define	GT_WRITE32(base, offset, value)					\
	*GT_ADDR32((base), (offset)) = (value)

static void
gt_pci_cs_read(struct pci_interface *pci, pci_tag_t tag,
	       pci_offset_t offset, pci_cs_data_t *datap)
{
	critical_enter();

	GT_WRITE32(pci->pci_cs_base, PCI_REG_CAUSE, 0);

	GT_WRITE32(pci->pci_cs_base, PCI_REG_CS_ADDR, 1 << 31 | tag | offset);
	*datap = GT_READ32(pci->pci_cs_base, PCI_REG_CS_DATA);

	if ((GT_READ32(pci->pci_cs_base, PCI_REG_CAUSE) &
	     (PCI_REG_CAUSE_MASTER_ABORT | PCI_REG_CAUSE_TARGET_ABORT)) != 0) {
		*datap = ~0;
	}

	critical_exit();
}

static void
gt_describe(struct bus_instance *bi)
{
	bus_printf(bi, "GT 64120");
}

static int
gt_enumerate_children(struct bus_instance *bi)
{
	struct bus_instance *child;
	struct pci_interface *pci;
	int error;

	error = bus_enumerate_child(bi, "pci", &child);
	if (error != 0)
		return (error);

	pci = bus_parent_data_allocate(child, sizeof *pci);

	pci->pci_cs_read = gt_pci_cs_read;

	pci->pci_io_enable = true;
	pci->pci_io_base = 0x18000000;

	pci->pci_memory_enable = true;
	pci->pci_memory_base = 0x10000000;

	pci->pci_cs_base = 0x1be00000;

	error = bus_setup_child(child);
	if (error != 0)
		return (error);

	return (0);
}

static int
gt_setup(struct bus_instance *bi)
{
	return (0);
}

BUS_INTERFACE(gtif) {
	.bus_describe = gt_describe,
	.bus_enumerate_children = gt_enumerate_children,
	.bus_setup = gt_setup,
};
BUS_ATTACHMENT(gt, "mainbus", gtif);
