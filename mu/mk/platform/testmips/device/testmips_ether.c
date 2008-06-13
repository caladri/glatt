#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>
#include <io/device/device.h>
#include <io/network/ethernet.h>
#include <io/network/interface.h>

#include <io/console/console.h>

#define	TEST_ETHER_DEV_BASE	(0x14000000)
#define	TEST_ETHER_DEV_IRQ	(3)

#define	TEST_ETHER_DEV_MTU	(0x4000)

#define	TEST_ETHER_DEV_BUFFER	(0x0000)
#define	TEST_ETHER_DEV_STATUS	(0x4000)
#define	TEST_ETHER_DEV_LENGTH	(0x4010)
#define	TEST_ETHER_DEV_COMMAND	(0x4020)
#define	TEST_ETHER_DEV_MAC	(0x4040)

#define	TEST_ETHER_DEV_FUNCTION(f)					\
	(volatile uint64_t *)XKPHYS_MAP(XKPHYS_UC, TEST_ETHER_DEV_BASE + (f))
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

static void
tmether_describe(struct device *device)
{
	device_printf(device, "testmips simulated ethernet device.");
}

static int
tmether_setup(struct device *device, void *busdata)
{
	struct tmether_softc *sc;
	int error;

	sc = device_softc_allocate(device, sizeof *sc);
	spinlock_init(&sc->sc_lock, "testmips ethernet", SPINLOCK_FLAG_DEFAULT);
	TMETHER_LOCK(sc);
	error = network_interface_attach(&sc->sc_netif,
					 NETWORK_INTERFACE_ETHERNET,
					 tmether_request, device);
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
	switch (TEST_ETHER_DEV_READ(TEST_ETHER_DEV_STATUS)) {
	case TEST_ETHER_DEV_STATUS_RX_OK:
		panic("%s: TEST_ETHER_DEV_STATUS_RX_OK", __func__);
		break;
	case TEST_ETHER_DEV_STATUS_RX_MORE:
		panic("%s: TEST_ETHER_DEV_STATUS_RX_MORE", __func__);
		break;
	default:
		panic("%s: unexpected status.", __func__);
		break;
	}
	TMETHER_UNLOCK(sc);
}

static int
tmether_request(void *softc, enum network_interface_request req,
		void *data, size_t datalen)
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

DEVICE_INTERFACE(tmetherif) {
	.device_describe = tmether_describe,
	.device_setup = tmether_setup,
};
DEVICE_ATTACHMENT(tmether, "mpbus", tmetherif);
