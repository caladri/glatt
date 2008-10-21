#ifndef	_IO_PCI_PCIDEV_H_
#define	_IO_PCI_PCIDEV_H_

struct pci_device {
	unsigned pd_bus;
	unsigned pd_slot;
	unsigned pd_function;
	unsigned pd_vendor;
	unsigned pd_device;
	/* XXX resources in use, etc.  */
};

#endif /* !_IO_PCI_PCIDEV_H_ */
