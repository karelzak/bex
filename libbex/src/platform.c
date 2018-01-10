
#include "bexP.h"

#include <libwebsockets.h>

static void free_platform(struct libbex_platform *pl)
{
	if (!pl)
		return;

	DBG(PLAT, bex_debugobj(pl, "free"));
	while (!list_empty(&pl->events)) {
		struct libbex_event *ev = list_entry(pl->events.next,
				                  struct libbex_event, events);
		bex_platform_remove_event(pl, ev);
	}
	free(pl->uri_path);
	free(pl->uri_addr);
	free(pl->uri_prot);
	free(pl);
}

/**
 * bex_new_platform:
 * @uri: platform address
 *
 * The initial refcount is 1, and needs to be decremented to
 * release the resources of the platform.
 *
 * Returns: newly allocated struct libbex_platform.
 */
struct libbex_platform *bex_new_platform(const char *uri)
{
	struct libbex_platform *pl = calloc(1, sizeof(*pl));
	char *_uri = NULL;
	const char *prot, *addr, *p = NULL;
	size_t sz = 0;

	if (!pl)
		goto err;
	DBG(PLAT, bex_debugobj(pl, "alloc"));

	/* libwebsocket modifies URI */
	if (!(_uri = strdup(uri)))
		goto err;

	pl->service_timeout = 250;
	pl->reconnect_timeout = 50;
	pl->connection_attempts = 5;
	pl->uri_port = 443;

	if (lws_parse_uri(_uri, &prot, &addr, &pl->uri_port, &p))
		goto err;

	/* path */
	if (p)
		sz = strlen(p);
	pl->uri_path = calloc(1, sz + 2);
	if (!pl->uri_path)
		goto err;

	pl->uri_path[0] = '/';
	memcpy(pl->uri_path + 1, p, sz);

	/* protocol */
	pl->uri_prot = strdup(prot);
	if (!pl->uri_prot)
		goto err;

	if (!strcmp(prot, "https") || !strcmp(prot, "wss"))
		pl->uri_ssl |= LCCSCF_USE_SSL;

	/* address */
	pl->uri_addr = strdup(addr);
	if (!pl->uri_addr)
		goto err;

	pl->refcount = 1;
	INIT_LIST_HEAD(&pl->events);

	DBG(PLAT, bex_debugobj(pl, "protocol=%s, address=%s, port=%d, path=%s [SSL=%s]",
				pl->uri_prot, pl->uri_addr,
				pl->uri_port, pl->uri_path,
				pl->uri_ssl ? "YES" : "NO"));
	free(_uri);
	return pl;
err:
	free(_uri);
	free_platform(pl);
	return NULL;
}

/**
 * bex_ref_platform:
 * @pl: platform pointer
 *
 * Increments reference counter.
 */
void bex_ref_platform(struct libbex_platform *pl)
{
	if (pl)
		pl->refcount++;
}

/**
 * bex_unref_platform:
 * @pl: event pointer
 *
 * De-increments reference counter, on zero the @pl is automatically
 * deallocated.
 */
void bex_unref_platform(struct libbex_platform *pl)
{
	if (pl) {
		pl->refcount--;
		if (pl->refcount <= 0)
			free_platform(pl);
	}
}

/**
 * bex_platform_add_event:
 * @pl: tab pointer
 * @ev: event
 *
 * Adds a new event to platform and increment @ev reference counter. Don't forget to
 * use bex_unref_event() after bex_platform_add_event() you want to keep the @ev
 * referenced by the platform only.
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_platform_add_event(struct libbex_platform *pl, struct libbex_event *ev)
{
	if (!pl || !ev)
		return -EINVAL;

	bex_ref_event(ev);
	list_add_tail(&ev->events, &pl->events);

	DBG(PLAT, bex_debugobj(pl, "add event: %s [%p]", ev->name, ev));
	return 0;
}

/**
 * bex_platform_remove_event:
 * @pl: platform pointer
 * @ev: event
 *
 * Removes the @ev from the platform and de-increment reference counter of the
 * @ev. The event with zero reference counter will be deallocated. Don't forget
 * to use bex_ref_event() before call bex_platform_remove_event() if you want
 * to use @ev later.
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_platform_remove_event(struct libbex_platform *pl, struct libbex_event *ev)
{
	if (!pl || !ev)
		return -EINVAL;

	DBG(PLAT, bex_debugobj(pl, "removing event %s [%p]", ev->name, ev));

	list_del(&ev->events);
	INIT_LIST_HEAD(&ev->events);	/* otherwise EV still points to the list */

	bex_unref_event(ev);
	return 0;
}


/**
 * bext_platform_next_event:
 * @pl: platform
 * @itr: iterator
 * @ev: returns the next event
 *
 * Returns: 0 on success, negative number in case of error or 1 at the end of list.
 */
int bex_platform_next_event(struct libbex_platform *pl, struct libbex_iter *itr, struct libbex_event **ev)
{
	int rc = 1;

	if (!pl || !itr || !ev)
		return -EINVAL;
	*ev = NULL;

	if (!itr->head)
		BEX_ITER_INIT(itr, &pl->events);
	if (itr->p != itr->head) {
		BEX_ITER_ITERATE(itr, *ev, struct libbex_event, events);
		rc = 0;
	}

	return rc;
}

struct libbex_event *bex_platform_get_event(struct libbex_platform *pl, const char *name)
{
	struct libbex_event *ev;
	struct libbex_iter itr;

	bex_reset_iter(&itr, BEX_ITER_FORWARD);

	while (bex_platform_next_event(pl, &itr, &ev) == 0) {
		if (strcmp(ev->name, name) == 0)
			return ev;
	}

	return NULL;
}

int bex_platform_emit_event(struct libbex_platform *pl, struct libbex_event *ev)
{
	int rc = 0;
	size_t sz = 0;
	char *str;
	FILE *stream;

	if (!ev || !pl)
		return -EINVAL;

	DBG(PLAT, bex_debugobj(pl, "emitting event %s [%p]", ev->name, ev));

	stream = open_memstream(&str, &sz);
	if (!stream)
		return -errno;

	/* convert to JSON string */
	fputs("{ ", stream);
	fprintf(stream, "\"event\": \"%s\"", ev->name);

	if (!bex_array_is_empty(ev->vals)) {
		fputs(", ", stream);
		rc = bex_array_to_stream(ev->vals, stream);
	}

	fputs(" }", stream);
        fclose(stream);

	if (!rc)
		rc = bex_platform_send(pl, (unsigned char *) str, sz);
	if (rc)
		free(str);	/* free on error */

	return rc;
}

int bex_platform_receive_event(struct libbex_platform *pl, struct libbex_event *ev,
			       struct libbex_array *ar)
{
	DBG(PLAT, bex_debugobj(pl, "received event %s [%p]", ev->name, ev));

	if (ev->callback)
		return ev->callback(pl, ev, ar);

	return 0;
}

int bex_platform_connect(struct libbex_platform *pl)
{
	DBG(PLAT, bex_debugobj(pl, "connecting"));
	return wss_connect(pl);;
}

int bex_platform_disconnect(struct libbex_platform *pl)
{
	DBG(PLAT, bex_debugobj(pl, "connecting"));
	return wss_disconnect(pl);;
}

int bex_platform_service(struct libbex_platform *pl)
{
	DBG(PLAT, bex_debugobj(pl, "serving"));
	return wss_service(pl);
}

/*
 * Note that @str has to be mallocated string and will be later freed by
 * platform. Don't call free() for the @str on success!
 */
int bex_platform_send(struct libbex_platform *pl, unsigned char *str, size_t sz)
{
	DBG(PLAT, bex_debugobj(pl, "sending: [sz=%zu] >>>%s<<<", sz, str));
	return wss_send(pl, str, sz);
}

static int is_event_string(const unsigned char *str, char **name)
{
	const char *p = (char *) str, *n;

	*name = NULL;

	p = skip_space(p);
	if (!*p != '{')
		return 0;

	p = skip_space(p);
	if (strcmp(p, "\"event\"") != 0)
		return 0;

	p = skip_space(p);
	if (!*p != ':')
		return 0;

	p = skip_space(p);
	if (!*p != '"' || *(p+1) == '\0')
		return 0;
	n = ++p;
	while (*p && *p != '"')
		p++;

	if (!*p)
		return 0;
	name = strndup(n, p - n);
	return name ? 1 : 0;
}

int bex_platform_receive(struct libbex_platform *pl, const char *str)
{
	char *name = NULL;
	int rc = 0;

	DBG(PLAT, bex_debugobj(pl, "receive"));

	if (is_event_string(str, &name)) {
		struct libbex_event *ev;

		DBG(PLAT, bex_debugobj(pl, "received event with name '%s'", name));

		ev = bex_platform_get_event(pl, name);
		if (ev) {
			bex_array_fill_from_string(ev->reply, str);
			rc = bex_platform_receive_event(pl, ev);
		} else
			DBG(PLAT, bex_debugobj(pl, "event unssuported [ignore]"));
	}

	free(name);
	return rc;
}
