#include <core/types.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/queue.h>
#include <core/scheduler.h>
#include <core/sleepq.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <core/vdae.h>

/*
 * XXX
 * To garbage collect dead lists and dead vdae, we should have a vdae_list
 * which sits around waiting for threads to call their exit functions, which
 * they should call when a flag is set in their private data which tells them
 * they are to exit.
 */

#define	VDAE_FLAG_DEFAULT	(0x00000000)	/* Default flags.  */
#define	VDAE_FLAG_RUNNING	(0x00000001)	/* vdae is running.  */
#define	VDAE_FLAG_WAKEUP	(0x00000002)	/* vdae is wakeup pending.  */

struct vdae {
	struct mutex v_mutex;
	struct thread *v_thread;
	unsigned v_flags;
	struct vdae_list *v_list;
	vdae_callback_t *v_callback;
	void *v_arg;
	TAILQ_ENTRY(struct vdae) v_link;
};

#define	VDAE_LOCK(v)		mutex_lock(&(v)->v_mutex)
#define	VDAE_UNLOCK(v)		mutex_unlock(&(v)->v_mutex)

struct vdae_list {
	struct mutex vl_mutex;
	const char *vl_name;
	struct task *vl_task;
	TAILQ_HEAD(, struct vdae) vl_list;
};

#define	VDAE_LIST_LOCK(vlist)	mutex_lock(&(vlist)->vl_mutex)
#define	VDAE_LIST_UNLOCK(vlist)	mutex_unlock(&(vlist)->vl_mutex)

static struct pool vdae_pool;
static struct pool vdae_list_pool;

static void vdae_thread_loop(void *);

struct vdae *
vdae_create(struct vdae_list *vlist, vdae_callback_t *callback, void *arg)
{
	struct vdae *v;
	int error;

	v = pool_allocate(&vdae_pool);
	mutex_init(&v->v_mutex, "VDAE");
	VDAE_LIST_LOCK(vlist);
	VDAE_LOCK(v);
	v->v_flags = VDAE_FLAG_DEFAULT;
	v->v_list = vlist;
	error = thread_create(&v->v_thread, vlist->vl_task, vlist->vl_name,
			      THREAD_DEFAULT);
	if (error != 0)
		panic("%s: thread_create failed: %m", __func__, error);
	TAILQ_INSERT_TAIL(&vlist->vl_list, v, v_link);
	VDAE_LIST_UNLOCK(vlist);
	thread_set_upcall(v->v_thread, vdae_thread_loop, v);
	scheduler_thread_runnable(v->v_thread);
	v->v_callback = callback;
	v->v_arg = arg;
	VDAE_UNLOCK(v);
	return (v);
}

struct vdae_list *
vdae_list_create(const char *name)
{
	struct vdae_list *vlist;
	int error;

	vlist = pool_allocate(&vdae_list_pool);
	mutex_init(&vlist->vl_mutex, "VDAE LIST");
	vlist->vl_name = name;
	error = task_create(&vlist->vl_task, NULL, vlist->vl_name, TASK_KERNEL);
	if (error != 0)
		panic("%s: task_create failed: %m", __func__, error);
	TAILQ_INIT(&vlist->vl_list);
	return (vlist);
}

void
vdae_list_wakeup(struct vdae_list *vlist)
{
	struct vdae *v;

	VDAE_LIST_LOCK(vlist);
	/*
	 * Wake up every thread but ourselves.  If we are waking up ourselves,
	 * and there is only one CPU, the other threads will be starved.
	 */
	TAILQ_FOREACH(v, &vlist->vl_list, v_link) {
		VDAE_LOCK(v);
		if (v->v_thread != current_thread())
			v->v_flags |= VDAE_FLAG_WAKEUP;
		VDAE_UNLOCK(v);
	}
	VDAE_LIST_UNLOCK(vlist);
	sleepq_signal(vlist);
}

static void
vdae_thread_loop(void *arg)
{
	struct vdae *v;

	v = arg;

	ASSERT(current_thread() == v->v_thread, "VDAE must be on its thread.");

	for (;;) {
		VDAE_LOCK(v);
		/*
		 * Use a while loop rather than an if, in case we get spurious
		 * wakeups from the sleepq.
		 */
		while ((v->v_flags & VDAE_FLAG_WAKEUP) == 0) {
			VDAE_UNLOCK(v);
			sleepq_wait(v->v_list);
			VDAE_LOCK(v);
		}
		v->v_flags |= VDAE_FLAG_RUNNING;
		v->v_flags &= ~VDAE_FLAG_WAKEUP;
		VDAE_UNLOCK(v);

		v->v_callback(v->v_arg);

		VDAE_LOCK(v);
		v->v_flags &= ~VDAE_FLAG_RUNNING;
		VDAE_UNLOCK(v);
	}
}

static void
vdae_startup(void *arg)
{
	int error;

	error = pool_create(&vdae_pool, "VDAE", sizeof (struct vdae),
			    POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_init failed: %m", __func__, error);
	error = pool_create(&vdae_list_pool, "VDAE LIST",
			    sizeof (struct vdae_list), POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_init failed: %m", __func__, error);
}
STARTUP_ITEM(vdae, STARTUP_POOL, STARTUP_FIRST, vdae_startup, NULL);
