#include <core/types.h>
#include <core/mp.h>
#include <core/startup.h>
#include <db/db.h>
#if defined(OLDDRIVER)
#include <io/device/device.h>
#include <io/device/driver.h>
#else
#include <cpu/interrupt.h>
#endif

void
platform_startup(void)
{
	cpu_startup();
	startup_main();
}

void
platform_startup_thread(void)
{
}

static void
platform_startup_cpu(void *arg)
{
#if defined(OLDDRIVER)
	struct device *device;
	struct driver *driver;
	int error;

	driver = driver_lookup("cpu");
	error = device_create(&device, NULL, driver);
	if (error != 0)
		panic("%s: device_create failed: %m", __func__, error);
#else
	cpu_interrupt_setup();
#endif
}
STARTUP_ITEM(platform_cpu, STARTUP_DRIVERS, STARTUP_FIRST,
	     platform_startup_cpu, NULL);
