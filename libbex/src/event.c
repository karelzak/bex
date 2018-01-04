
#include "bexP.h"
static void free_event(struct libbex_event *ev)
{
	if (!ev)
		return;

	free(ev->name);
	free(ev);
}

/**
 * bex_new_event:
 * @name: event name
 *
 * The initial refcount is 1, and needs to be decremented to
 * release the resources of the filesystem.
 *
 * Returns: newly allocated struct libbex_event.
 */
struct libbex_event *bex_new_event(const char *name)
{
	struct libbex_event *ev = calloc(1, sizeof(*ev));
	if (!ev)
		goto err;

	ev->refcount = 1;
	ev->name = strdup(name);
	if (!ev->name)
		goto err;
	INIT_LIST_HEAD(&ev->events);
	return ev;
err:
	free_event(ev);
	return NULL;
}

/**
 * bex_ref_event:
 * @ev: event pointer
 *
 * Increments reference counter.
 */
void bex_ref_event(struct libbex_event *ev)
{
	if (ev)
		ev->refcount++;
}

/**
 * bex_unref_event:
 * @ev: event pointer
 *
 * De-increments reference counter, on zero the @ev is automatically
 * deallocated.
 */
void bex_unref_event(struct libbex_event *ev)
{
	if (ev) {
		ev->refcount--;
		if (ev->refcount <= 0)
			free_event(ev);
	}
}
