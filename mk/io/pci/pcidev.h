#ifndef	_IO_PCI_PCIDEV_H_
#define	_IO_PCI_PCIDEV_H_

struct device_attachment;
struct bus_attachment;

/*
 * XXX
 * Multi-function devices?
 * Domains.
 */

struct pci_device {
	unsigned pd_bus;
	unsigned pd_slot;
	unsigned pd_function;
	unsigned pd_vendor;
	unsigned pd_device;
	/* XXX resources in use, etc.  */
};

enum pci_attachment_type {
	PCI_ATTACHMENT_BRIDGE,
	PCI_ATTACHMENT_DEVICE,
};

struct pci_attachment {
	unsigned short pa_vendor;
	unsigned short pa_device;
	enum pci_attachment_type pa_type;
	union pci_attachment_driver {
		struct device_attachment *pad_device;
		struct bus_attachment *pad_bus;
	} pa_driver;
};

#define	PCI_BRIDGE(dev, ifname, vendor, device)				\
	BUS_ATTACHMENT(dev, "pci", ifname);				\
									\
	struct pci_attachment __pci_attachment_ ## dev= {		\
		.pa_vendor = vendor,					\
		.pa_device = device,					\
		.pa_type = PCI_ATTACHMENT_BRIDGE,			\
		.pa_driver.pad_bus = &__bus_attachment_ ## dev,		\
	};								\
	SET_ADD(pci_attachments, __pci_attachment_ ## dev)

#define	PCI_DEVICE(dev, ifname, vendor, device)				\
	DEVICE_ATTACHMENT(dev, "pci", ifname);				\
									\
	struct pci_attachment __pci_attachment_ ## dev = {		\
		.pa_vendor = vendor,					\
		.pa_device = device,					\
		.pa_type = PCI_ATTACHMENT_DEVICE,			\
		.pa_driver.pad_device = &__device_attachment_ ## dev,	\
	};								\
	SET_ADD(pci_attachments, __pci_attachment_ ## dev)

extern struct pci_attachment __pci_attachment_pcidev;

#endif /* !_IO_PCI_PCIDEV_H_ */
