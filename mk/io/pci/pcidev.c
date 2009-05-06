#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <io/device/device.h>
#include <io/pci/pcidev.h>

static void
pcidev_describe(struct device *device)
{
	struct pci_device *pcidev = device_softc(device);

	device_printf(device, "PCI device (bus/slot/function=%u/%u/%u, vendor=%x, device=%x, class=%x, subclass=%x)", pcidev->pd_bus, pcidev->pd_slot, pcidev->pd_function, pcidev->pd_vendor, pcidev->pd_device, pcidev->pd_class, pcidev->pd_subclass);
}

static int
pcidev_setup(struct device *device, void *busdata)
{
	struct pci_device *pcidev;

	pcidev = device_softc_allocate(device, sizeof *pcidev);
	memcpy(pcidev, busdata, sizeof *pcidev);

	return (0);
}

DEVICE_INTERFACE(pcidevif) {
	.device_describe = pcidev_describe,
	.device_setup = pcidev_setup,
};
PCI_DEVICE(pcidev, pcidevif, PCIDEV_VENDOR_ANY, PCIDEV_DEVICE_ANY,
	   PCIDEV_CLASS_ANY, PCIDEV_SUBCLASS_ANY);
