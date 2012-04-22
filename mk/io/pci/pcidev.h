#ifndef	_IO_PCI_PCIDEV_H_
#define	_IO_PCI_PCIDEV_H_

struct bus_attachment;

struct pci_device {
	unsigned pd_domain;
	unsigned pd_bus;
	unsigned pd_slot;
	unsigned pd_function;
	unsigned pd_vendor;
	unsigned pd_device;
	unsigned pd_class;
	unsigned pd_subclass;
};

enum pci_attachment_type {
	PCI_ATTACHMENT_BRIDGE,
	PCI_ATTACHMENT_DEVICE,
};

/*
 * <vendor, device> definitions.
 */
#define	PCIDEV_VENDOR_ANY	0xffff
#define	PCIDEV_DEVICE_ANY	0xffff

struct pci_attachment {
	unsigned short pa_vendor;
	unsigned short pa_device;
	struct bus_attachment *pa_attachment;
};

#define	PCI_ATTACHMENT(dev, ifname, vendor, device)			\
	BUS_ATTACHMENT(dev, "pci", ifname);				\
									\
	struct pci_attachment __pci_attachment_ ## dev= {		\
		.pa_vendor = vendor,					\
		.pa_device = device,					\
		.pa_attachment = &__bus_attachment_ ## dev,		\
	};								\
	SET_ADD(pci_attachments, __pci_attachment_ ## dev)

extern struct pci_attachment __pci_attachment_pcidev;

#endif /* !_IO_PCI_PCIDEV_H_ */
