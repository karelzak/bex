
#include "bexP.h"
static void free_event(struct libbex_event *ev)
{
	if (!ev)
		return;

	while (!list_empty(&ev->vals)) {
		struct libbex_value *va = list_entry(ev->vals.next,
				                  struct libbex_vals, vals);
		bex_event_remove_value(pl, va);
	}
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
	INIT_LIST_HEAD(&ev->vals);
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


/**
 * bex_platform_add_value:
 * @ev: event
 * @va: value
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_event_add_value(struct libbex_event *ev, struct libbex_value *va)
{
	if (!va || !ev)
		return -EINVAL;

	bex_ref_value(va);
	list_add_tail(&va->vals, &ev->vals);

	DBG(EVENT, bex_debugobj(pl, "add value: %s", va->name));
	return 0;
}

/**
 * bex_event_remove_value:
 * @ev: event
 * @val: value
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_platform_remove_value(struct libbex_platform *pl, struct libbex_value *va)
{
	if (!pl || !ev)
		return -EINVAL;

	list_del(&va->vals);
	INIT_LIST_HEAD(&va->vals);	/* otherwise EV still points to the list */

	bex_unref_value(va);
	return 0;
}
