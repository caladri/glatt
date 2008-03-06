#ifndef	_IO_PCI_PCI_H_
#define	_IO_PCI_PCI_H_

typedef	uint32_t	pci_tag_t;
typedef	unsigned int	pci_bus_t;
typedef	unsigned int	pci_slot_t;
typedef unsigned int	pci_function_t;

typedef	uint64_t	pci_offset_t;
typedef	uint64_t	pci_base_t;

typedef	uint32_t	pci_cs_data_t;

struct pci_interface {
	/* Configuration space methods.  */
	void (*pci_cs_read)(struct pci_interface *, pci_tag_t, pci_offset_t,
			    pci_cs_data_t *);

	/* Addresses, etc.  */
	bool pci_io_enable;
	pci_base_t pci_io_base;

	bool pci_memory_enable;
	pci_base_t pci_memory_base;

	pci_base_t pci_cs_base;
};

#endif /* !_IO_PCI_PCI_H_ */
