#include <core/types.h>
#include <core/error.h>
#include <io/device/bus.h>
#include <io/pci/pcidev.h>

static int
pcidev_enumerate_children(struct bus_instance *bi)
{
	return (0);
}

static int
pcidev_setup(struct bus_instance *bi)
{
	struct pci_device *pcidev;

	pcidev = bus_parent_data(bi);

	bus_set_description(bi, "PCI device class=%x subclass=%x",
			    pcidev->pd_class, pcidev->pd_subclass);

	return (0);
}

BUS_INTERFACE(pcidevif) {
	.bus_enumerate_children = pcidev_enumerate_children,
	.bus_setup = pcidev_setup,
};
PCI_ATTACHMENT(pcidev, pcidevif, PCIDEV_VENDOR_ANY, PCIDEV_DEVICE_ANY);
