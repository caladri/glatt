#ifndef	_IO_PCI_PCIREG_H_
#define	_IO_PCI_PCIREG_H_

/* Bus configuration registers.  */
#define	PCI_REG_CAUSE		0x0c18
#define	PCI_REG_CAUSE_MASTER_ABORT	0x00040000
#define	PCI_REG_CAUSE_TARGET_ABORT	0x00080000
#define	PCI_REG_CS_ADDR		0x0cf8
#define	PCI_REG_CS_DATA		0x0cfc

/* Device configuration registers.  */
#define	PCI_REG_CS_DEVID	0x0000
#define	PCI_REG_CS_CLASS	0x0008

#endif /* !_IO_PCI_PCIREG_H_ */
