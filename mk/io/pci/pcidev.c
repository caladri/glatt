#include <core/types.h>
#include <core/error.h>
#include <io/device/bus.h>
#include <io/pci/pcidev.h>

static void
pcidev_describe(struct bus_instance *bi)
{
	struct pci_device *pcidev;

	pcidev = bus_parent_data(bi);

	bus_printf(bi, "PCI-device attachment (bus/slot/function=%u/%u/%u, vendor=%x, device=%x)", pcidev->pd_bus, pcidev->pd_slot, pcidev->pd_function, pcidev->pd_vendor, pcidev->pd_device);
}

static int
pcidev_enumerate_children(struct bus_instance *bi)
{
	/* XXX */
	return (0);
}

static int
pcidev_setup(struct bus_instance *bi)
{
	return (0);
}

BUS_INTERFACE(pcidevif) {
	.bus_describe = pcidev_describe,
	.bus_enumerate_children = pcidev_enumerate_children,
	.bus_setup = pcidev_setup,
};
BUS_ATTACHMENT(pcidev, "pci", pcidevif);
