#include <core/types.h>
#include <core/startup.h>
#include <core/thread.h>
#include <core/vdae.h>

#include <io/device/console/console.h>

static void
test_vdae_callback(void *arg)
{
	static int i;
	struct vdae_list *vlist;

	vlist = arg;

	vdae_list_wakeup(vlist);

	kcprintf("%p got the ball (%d) on cpu%u!\n", current_thread(), i++, mp_whoami());
}

static void
test_vdae_startup(void *arg)
{
#define	NPLAYERS	32
	struct vdae_list *vlist;
	struct vdae *v;
	unsigned i;

	kcprintf("Setting up a nice ball game...\n");
	vlist = vdae_list_create("ball game");
	for (i = 0; i < NPLAYERS; i++) {
		v = vdae_create(vlist, test_vdae_callback, vlist);
	}
	vdae_list_wakeup(vlist);
}
STARTUP_ITEM(test_vdae, STARTUP_MAIN, STARTUP_BEFORE, test_vdae_startup, NULL);
