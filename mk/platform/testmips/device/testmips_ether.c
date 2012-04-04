#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>
#include <io/bus/bus.h>
#include <io/network/ethernet.h>
#include <io/network/interface.h>

#define	TEST_ETHER_DEV_BASE	(0x14000000)
#define	TEST_ETHER_DEV_IRQ	(3)

#define	TEST_ETHER_DEV_MTU	(0x4000)

#define	TEST_ETHER_DEV_BUFFER	(0x0000)
#define	TEST_ETHER_DEV_STATUS	(0x4000)
#define	TEST_ETHER_DEV_LENGTH	(0x4010)
#define	TEST_ETHER_DEV_COMMAND	(0x4020)
#define	TEST_ETHER_DEV_MAC	(0x4040)

#define	TEST_ETHER_DEV_FUNCTION(f)					\
	(volatile uint64_t *)XKPHYS_MAP(CCA_UC, TEST_ETHER_DEV_BASE + (f))
#define	TEST_ETHER_DEV_READ(f)						\
	(volatile uint64_t)*TEST_ETHER_DEV_FUNCTION(f)
#define	TEST_ETHER_DEV_WRITE(f, v)					\
	*TEST_ETHER_DEV_FUNCTION(f) = (v)

#define	TEST_ETHER_DEV_STATUS_RX_OK	(0x01)
#define	TEST_ETHER_DEV_STATUS_RX_MORE	(0x02)

#define	TEST_ETHER_DEV_COMMAND_RX	(0x00)
#define	TEST_ETHER_DEV_COMMAND_TX	(0x01)

struct tmether_softc {
	struct spinlock sc_lock;
	struct network_interface sc_netif;
};

#define	TMETHER_LOCK(sc)	spinlock_lock(&(sc)->sc_lock)
#define	TMETHER_UNLOCK(sc)	spinlock_unlock(&(sc)->sc_lock)

static void tmether_interrupt(void *, int);
static network_interface_request_handler_t tmether_request;
static network_interface_transmit_t tmether_transmit;

static int
tmether_setup(struct bus_instance *bi)
{
	struct tmether_softc *sc;
	int error;

	bus_set_description(bi, "testmips simulated ethernet device.");

	sc = bus_softc_allocate(bi, sizeof *sc);
	spinlock_init(&sc->sc_lock, "testmips ethernet", SPINLOCK_FLAG_DEFAULT);
	TMETHER_LOCK(sc);
	error = network_interface_attach(&sc->sc_netif,
					 NETWORK_INTERFACE_ETHERNET, "tmether0",
					 tmether_request, tmether_transmit, sc);
	if (error != 0) {
		TMETHER_UNLOCK(sc);
		return (error);
	}
	cpu_interrupt_establish(TEST_ETHER_DEV_IRQ, tmether_interrupt, sc);
	TMETHER_UNLOCK(sc);
	return (0);
}

static void
tmether_interrupt(void *arg, int interrupt)
{
	struct tmether_softc *sc = arg;

	TMETHER_LOCK(sc);
	for (;;) {
		uint64_t status, length;

		/*
		 * XXX
		 * Limit number of packets received at once?
		 */
		status = TEST_ETHER_DEV_READ(TEST_ETHER_DEV_STATUS);
		if (status == TEST_ETHER_DEV_STATUS_RX_MORE) {
			TEST_ETHER_DEV_WRITE(TEST_ETHER_DEV_COMMAND, TEST_ETHER_DEV_COMMAND_RX);
			continue;
		}
		if (status != TEST_ETHER_DEV_STATUS_RX_OK)
			break;
		length = TEST_ETHER_DEV_READ(TEST_ETHER_DEV_LENGTH);
		network_interface_receive(&sc->sc_netif, XKPHYS_MAP(CCA_UC, TEST_ETHER_DEV_BASE + TEST_ETHER_DEV_BUFFER), length);
	}
	TMETHER_UNLOCK(sc);
}

static int
tmether_request(void *softc, enum network_interface_request req, void *data,
		size_t datalen)
{
	struct tmether_softc *sc = softc;

	switch (req) {
	case NETWORK_INTERFACE_GET_ADDRESS:
		if (datalen != ETHERNET_MAC_LENGTH)
			return (ERROR_INVALID);
		TMETHER_LOCK(sc);
		TEST_ETHER_DEV_WRITE(TEST_ETHER_DEV_MAC, (uintptr_t)data);
		TMETHER_UNLOCK(sc);
		return (0);
	default:
		return (ERROR_NOT_IMPLEMENTED);
	}
}

static int
tmether_transmit(void *softc, const void *data, size_t datalen)
{
	struct tmether_softc *sc = softc;

	if (data == NULL || datalen == 0 || datalen > TEST_ETHER_DEV_MTU)
		return (ERROR_INVALID);

	TMETHER_LOCK(sc);
	TEST_ETHER_DEV_WRITE(TEST_ETHER_DEV_LENGTH, datalen);
	memcpy(XKPHYS_MAP(CCA_UC, TEST_ETHER_DEV_BASE + TEST_ETHER_DEV_BUFFER), data, datalen);
	TEST_ETHER_DEV_WRITE(TEST_ETHER_DEV_COMMAND, TEST_ETHER_DEV_COMMAND_TX);
	TMETHER_UNLOCK(sc);

	return (0);
}

BUS_INTERFACE(tmetherif) {
	.bus_setup = tmether_setup,
};
BUS_ATTACHMENT(tmether, "mpbus", tmetherif);
