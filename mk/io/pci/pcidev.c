#include <core/types.h>
#include <core/error.h>
#include <io/device/bus.h>
#include <io/pci/pcidev.h>

static void
pcidev_describe(struct bus_instance *bi)
{
	struct pci_device *pcidev;

	pcidev = bus_parent_data(bi);

	bus_printf(bi, "PCI device (bus/slot/function=%u/%u/%u, vendor=%x, device=%x, class=%x, subclass=%x)", pcidev->pd_bus, pcidev->pd_slot, pcidev->pd_function, pcidev->pd_vendor, pcidev->pd_device, pcidev->pd_class, pcidev->pd_subclass);
}

static int
pcidev_enumerate_children(struct bus_instance *bi)
{
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
PCI_BRIDGE(pcidev, pcidevif, PCIDEV_VENDOR_ANY, PCIDEV_DEVICE_ANY);
