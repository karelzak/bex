
#include "bexP.h"
static void free_event(struct libbex_event *ev)
{
	if (!ev)
		return;

	bex_unref_array(ev->vals);
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
	ev->vals = bex_new_array(3);
	if (!ev->vals)
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


/**
 * bex_event_add_value:
 * @ev: event
 * @va: value
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_event_add_value(struct libbex_event *ev, struct libbex_value *va)
{
	if (!va || !ev)
		return -EINVAL;
	return bex_array_add(ev->vals, va);
}

/**
 * bex_event_remove_value:
 * @ev: event
 * @val: value
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_event_remove_value(struct libbex_event *ev, struct libbex_value *va)
{
	if (!va || !ev)
		return -EINVAL;
	return bex_array_remove(ev->vals, va);
}

/**
 * bex_event_get_values
 * @ev: event
 *
 * Returns: values list
 */
struct libbex_array *bex_event_get_values(struct libbex_event *ev)
{
	if (!ev)
		return NULL;
	return ev->vals;
}

/**
 * bex_event_set_callback
 * @ev: event
 * @fn: callback function
 *
 * Returns: 0 or <0 on error
 */
int bex_event_set_callback(struct libbex_event *ev,
		int (*fn)(struct libbex_platform *, struct libbex_event *, void *))
{
	if (!ev)
		return NULL;
	return ev->callback = fn
}

/**
 * bex_event_set_data
 * @ev: event
 * @dt: data
 *
 * Returns: 0 or <0 on error
 */
int bex_event_set_data(struct libbex_event *ev, void *dt)
{
	if (!ev)
		return NULL;
	return ev->data = dt;
}

/**
 *
 * bex_event_get_data
 * @ev: event
 *
 * Returns: 0 or <0 on error
 */
void *bex_event_get_data(struct libbex_event *ev)
{
	if (!ev)
		return NULL;
	return ev->data;
}
