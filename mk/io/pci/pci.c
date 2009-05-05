#include <core/types.h>
#include <core/critical.h>
#include <core/error.h>
#include <io/device/bus.h>
#include <io/device/device.h>
#include <io/pci/pci.h>
#include <io/pci/pcidev.h>
#include <io/pci/pcireg.h>

SET(pci_attachments, struct pci_attachment);

struct pci_softc {
	struct bus_instance *sc_instance;
	struct pci_interface *sc_interface;
};

static void pci_attachment_find(struct pci_attachment **, uint32_t, uint32_t);
static int pci_enumerate_child(struct pci_softc *, pci_bus_t, pci_slot_t,
			       pci_function_t, pci_cs_data_t);
static pci_tag_t pci_tag_make(pci_bus_t, pci_slot_t, pci_function_t);

static void
pci_describe(struct bus_instance *bi)
{
	bus_printf(bi, "pci bus");
}

static int
pci_slot_walk(struct pci_softc *sc, pci_bus_t bus, pci_slot_t slot)
{
	pci_function_t function;
	pci_cs_data_t data;
	pci_tag_t tag;
	int error;

	for (function = 0; function < 8; function++) {
		tag = pci_tag_make(bus, slot, function);

		sc->sc_interface->pci_cs_read(sc->sc_interface, tag,
					      PCI_REG_CS_DEVID, &data);
		if (data == (pci_cs_data_t)~0)
			continue;
		error = pci_enumerate_child(sc, bus, slot, function, data);
		if (error != 0)
			panic("%s: pci_enumerate_child failed: %m",
			      __func__, error);
	}
	return (0);
}

static int
pci_bus_walk(struct pci_softc *sc, pci_bus_t bus)
{
	pci_slot_t slot;
	int error;

	for (slot = 0; slot < 32; slot++) {
		error = pci_slot_walk(sc, bus, slot);
		if (error != 0)
			panic("%s: pci_slot_walk failed: %m", __func__, error);
	}
	return (0);
}

static int
pci_enumerate_children(struct bus_instance *bi)
{
	struct pci_softc *sc;
	pci_bus_t bus;
	int error;

	sc = bus_softc(bi);

	for (bus = 0; bus < 256; bus++) {
		error = pci_bus_walk(sc, bus);
		if (error != 0)
			panic("%s: pci_bus_walk failed: %m", __func__, error);
	}

	return (0);
}

static int
pci_setup(struct bus_instance *bi)
{
	struct pci_softc *sc;

	sc = bus_softc_allocate(bi, sizeof *sc);

	sc->sc_instance = bi;
	sc->sc_interface = bus_parent_data(bi);

	return (0);
}

BUS_INTERFACE(pciif) {
	.bus_describe = pci_describe,
	.bus_enumerate_children = pci_enumerate_children,
	.bus_setup = pci_setup,
};
BUS_ATTACHMENT(pci, NULL, pciif);

static void
pci_attachment_find(struct pci_attachment **attachment2p, uint32_t vendor,
		    uint32_t device)
{
	struct pci_attachment **attachmentp;

	SET_FOREACH(attachmentp, pci_attachments) {
		struct pci_attachment *attachment = *attachmentp;

		if (attachment->pa_vendor != vendor)
			continue;
		if (attachment->pa_device != device)
			continue;
		*attachment2p = attachment;
		return;
	}

	/*
	 * XXX
	 * Scan for attachments by class?
	 */

	*attachment2p = &__pci_attachment_pcidev;
	return;
}

/*
 * XXX domain.
 */
static int
pci_enumerate_child(struct pci_softc *sc, pci_bus_t bus, pci_slot_t slot,
		    pci_function_t function, pci_cs_data_t id)
{
	struct bus_instance *child;
	struct pci_attachment *attachment;
	struct pci_device pd, *pcidev;
	const char *driver;
	int error;

	pd.pd_bus = bus;
	pd.pd_slot = slot;
	pd.pd_function = function;
	pd.pd_vendor = id & 0xffff;
	pd.pd_device = (id >> 16) & 0xffff;

	pci_attachment_find(&attachment, pd.pd_vendor, pd.pd_device);

	switch (attachment->pa_type) {
	case PCI_ATTACHMENT_BRIDGE:
		driver = attachment->pa_driver.pad_bus->ba_name;
		error = bus_enumerate_child(sc->sc_instance, driver, &child);
		if (error != 0)
			return (error);

		pcidev = bus_parent_data_allocate(child, sizeof *pcidev);
		*pcidev = pd;

		error = bus_setup_child(child);
		if (error != 0)
			return (error);
		break;

	case PCI_ATTACHMENT_DEVICE:
		driver = attachment->pa_driver.pad_device->da_name;
		error = device_enumerate(sc->sc_instance, driver, &pd);
		if (error != 0)
			return (error);
		break;
	}

	return (0);
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
