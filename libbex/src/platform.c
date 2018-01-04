
#include "bexP.h"

static void free_platform(struct libbex_platform *pl)
{
	if (!pl)
		return;

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


	/* libwebsocket modifies URI */
	if ((_uri = strdup(uri)))
		goto err;

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

	DBG(PLAT, bex_debugobj("protocol=%s, address=%s, port=%d, path=%s [SSL=%s]",
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

	list_del(&ev->events);
	INIT_LIST_HEAD(&ev->events);	/* otherwise EV still points to the list */

	bex_unref_event(ev);
	return 0;
}

