
#include "bexP.h"
#include "strutils.h"

static void free_event(struct libbex_event *ev)
{
	if (!ev)
		return;

	DBG(EVENT, bex_debugobj(ev, "free [name=%s]", ev->name));
	bex_unref_array(ev->vals);
	bex_unref_array(ev->reply);
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

	DBG(EVENT, bex_debugobj(ev, "alloc [name=%s]", name));
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

	if (!ev->vals) {
		ev->vals = bex_new_array(3);
		if (!ev->vals)
			return -ENOMEM;
	}

	DBG(EVENT, bex_debugobj(ev, "add value %s [%p]", va->name, va));
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
	if (!ev->vals)
		return 0;

	DBG(EVENT, bex_debugobj(ev, "remove value %s [%p]", va->name, va));
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
 * bex_event_get_replies
 * @ev: event
 *
 * Returns: values list
 */
struct libbex_array *bex_event_get_replies(struct libbex_event *ev)
{
	if (!ev)
		return NULL;
	return ev->reply;
}

/**
 * bex_event_set_reply_callback
 * @ev: event
 * @fn: callback function
 *
 * Returns: 0 or <0 on error
 */
int bex_event_set_reply_callback(struct libbex_event *ev,
		int (*fn)(struct libbex_platform *, struct libbex_event *))
{
	if (!ev)
		return -EINVAL;
	DBG(EVENT, bex_debugobj(ev, "setting callback"));
	ev->callback = fn;
	return 0;
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
		return -EINVAL;
	ev->data = dt;
	return 0;
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

/**
 * bex_event_add_reply:
 * @ev: event
 * @va: value
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_event_add_reply(struct libbex_event *ev, struct libbex_value *va)
{
	if (!va || !ev)
		return -EINVAL;

	DBG(EVENT, bex_debugobj(ev, "add reply %s [%p]", va->name, va));
	if (!ev->reply) {
		ev->reply = bex_new_array(3);
		if (!ev->reply)
			return -ENOMEM;
	}

	return bex_array_add(ev->reply, va);
}

/**
 * bex_event_remove_reply:
 * @ev: event
 * @val: value
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_event_remove_reply(struct libbex_event *ev, struct libbex_value *va)
{
	if (!va || !ev)
		return -EINVAL;
	if (!ev->reply)
		return 0;

	DBG(EVENT, bex_debugobj(ev, "remove reply %s [%p]", va->name, va));
	return bex_array_remove(ev->reply, va);
}

/**
 * bex_event_update_reply:
 * @ev: event
 * @str: unparsed data
 *
 * Parse @str and fill reply array.
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_event_update_reply(struct libbex_event *ev, const char *str)
{
	if (!ev || !str)
		return -EINVAL;

	DBG(EVENT, bex_debugobj(ev, "updating reply"));
	return bex_array_fill_from_string(ev->reply, str);
}

/**
 * bex_event_reset_reply:
 * @ev: event
 *
 * Removes on the fly-generated variables and zeroize another variables.
 */
void bex_event_reset_reply(struct libbex_event *ev)
{
	DBG(EVENT, bex_debugobj(ev, "reseting reply"));
	bex_reset_array(ev->reply);
}


int bex_is_event_string(const char *str, char **name)
{
	const char *p = (char *) str, *n;

	*name = NULL;

	p = skip_space(p);
	if (*p != '{')
		return 0;

	p = skip_space(++p);
	if (strncmp(p, "\"event\"", 7) != 0)
		return 0;
	p += 7;

	p = skip_space(p);
	if (*p != ':')
		return 0;

	p = skip_space(++p);
	if (*p != '"')
		return 0;

	n = ++p;
	while (*p && *p != '"')
		p++;

	if (!*p)
		return 0;

	*name = strndup(n, p - n);
	return *name ? 1 : 0;
}
