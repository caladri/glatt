#include <core/types.h>
#include <core/critical.h>
#include <core/error.h>
#include <db/db.h>
#include <io/device/bus.h>
#include <io/pci/pci.h>
#include <io/pci/pcireg.h>

struct pci_softc {
	struct pci_interface *sc_interface;
};

static int
pci_enumerate_child(struct pci_softc *sc, pci_bus_t bus, pci_slot_t slot,
		    pci_function_t function, pci_cs_data_t id)
{
	return (ERROR_NOT_IMPLEMENTED);
}

static pci_tag_t
pci_tag_make(pci_bus_t bus, pci_slot_t slot, pci_function_t function)
{
	pci_tag_t tag;

	bus &= 0xff;
	slot &= 0x1f;
	function &= 0x07;

	tag = (bus << 16) | (slot << 11) | (function << 8);
	return (tag);
}

static void
pci_describe(struct bus_instance *bi)
{
	bus_printf(bi, "pci bus");
}

static int
pci_enumerate_children(struct bus_instance *bi)
{
	struct pci_softc *sc;
	pci_bus_t bus;
	pci_slot_t slot;
	pci_function_t function;
	int error;

	sc = bus_softc(bi);

	bus = 0;

	for (slot = 0; slot < 32; slot++) {
		pci_cs_data_t data;
		pci_tag_t tag;

		function = 0;

		tag = pci_tag_make(bus, slot, function);

		sc->sc_interface->pci_cs_read(sc->sc_interface, tag,
					      PCI_REG_CS_DEVID, &data);
		if (data == (pci_cs_data_t)~0)
			continue;
		error = pci_enumerate_child(sc, bus, slot, function, data);
		if (error != 0)
			bus_printf(bi, "could not attach device %x", data);
	}

	return (0);
}

static int
pci_setup(struct bus_instance *bi, void *busdata)
{
	struct pci_softc *sc;

	sc = bus_softc_allocate(bi, sizeof *sc);

	sc->sc_interface = busdata;

	return (0);
}

BUS_INTERFACE(pciif) {
	.bus_describe = pci_describe,
	.bus_enumerate_children = pci_enumerate_children,
	.bus_setup = pci_setup,
};
BUS_ATTACHMENT(pci, NULL, pciif);
