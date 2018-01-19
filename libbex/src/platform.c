
#include "bexP.h"
#include "strutils.h"

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

	while (!list_empty(&pl->channels)) {
		struct libbex_channel *ch = list_entry(pl->channels.next,
				                  struct libbex_channel, channels);
		bex_platform_remove_channel(pl, ch);
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
	pl->reconnect_timeout = 500;
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
	INIT_LIST_HEAD(&pl->channels);

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

int bex_platform_set_timeout(struct libbex_platform *pl, int ms)
{
	pl->service_timeout = ms;
	return 0;
}

const char *bex_platform_get_address(struct libbex_platform *pl)
{
	return pl->uri_addr;
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
 * @pl: platform pointer
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

int bex_platform_send_event(struct libbex_platform *pl, struct libbex_event *ev)
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

int bex_platform_receive_event(struct libbex_platform *pl, struct libbex_event *ev)
{
	DBG(PLAT, bex_debugobj(pl, "received event %s [%p]", ev->name, ev));

	if (ev->callback)
		return ev->callback(pl, ev);

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

static int is_event_string(const char *str, char **name)
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

int bex_platform_receive(struct libbex_platform *pl, const char *str)
{
	char *name = NULL;
	int rc = 0;

	DBG(PLAT, bex_debugobj(pl, "receive: >>>%s<<<", str));

	if (is_event_string(str, &name)) {
		struct libbex_event *ev;

		DBG(PLAT, bex_debugobj(pl, "received event with name '%s'", name));

		ev = bex_platform_get_event(pl, name);
		if (ev) {
			rc = bex_event_update_reply(ev, str);
			if (!rc)
				rc = bex_platform_receive_event(pl, ev);
		} else
			DBG(PLAT, bex_debugobj(pl, "event unssuported [ignore]"));

	}

	free(name);
	return rc;
}

/**
 * bex_platform_add_channel:
 * @pl: tab pointer
 * @ch: channel
 *
 * Adds a new channel to platform and increment @ch reference counter. Don't forget to
 * use bex_unref_channel() after bex_platform_add_channel() you want to keep the @ch
 * referenced by the platform only.
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_platform_add_channel(struct libbex_platform *pl, struct libbex_channel *ch)
{
	if (!pl || !ch)
		return -EINVAL;

	bex_ref_channel(ch);
	list_add_tail(&ch->channels, &pl->channels);

	DBG(PLAT, bex_debugobj(pl, "add channel: %s [%p]", ch->name, ch));
	return 0;
}

/**
 * bex_platform_remove_channel:
 * @pl: platform pointer
 * @ev: channel
 *
 * Removes the @ch from the platform and de-increment reference counter of the
 * @ev. The channel with zero reference counter will be deallocated. Don't forget
 * to use bex_ref_channel() before call bex_platform_remove_channel() if you want
 * to use @ch later.
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_platform_remove_channel(struct libbex_platform *pl, struct libbex_channel *ch)
{
	if (!pl || !ch)
		return -EINVAL;

	DBG(PLAT, bex_debugobj(pl, "removing channel %s [%p]", ch->name, ch));

	list_del(&ch->channels);
	INIT_LIST_HEAD(&ch->channels);	/* otherwise @ch still points to the list */

	bex_unref_channel(ch);
	return 0;
}


/**
 * bext_platform_next_channel:
 * @pl: platform
 * @itr: iterator
 * @ch: returns the next channel
 *
 * Returns: 0 on success, negative number in case of error or 1 at the end of list.
 */
int bex_platform_next_channel(struct libbex_platform *pl, struct libbex_iter *itr,
			      struct libbex_channel **ch)
{
	int rc = 1;

	if (!pl || !itr || !ch)
		return -EINVAL;
	*ch = NULL;

	if (!itr->head)
		BEX_ITER_INIT(itr, &pl->channels);
	if (itr->p != itr->head) {
		BEX_ITER_ITERATE(itr, *ch, struct libbex_channel, channels);
		rc = 0;
	}

	return rc;
}

struct libbex_channel *bex_platform_get_channel(struct libbex_platform *pl, const char *name)
{
	struct libbex_channel *ch;
	struct libbex_iter itr;

	bex_reset_iter(&itr, BEX_ITER_FORWARD);

	while (bex_platform_next_channel(pl, &itr, &ch) == 0) {
		if (strcmp(ch->name, name) == 0)
			return ch;
	}

	return NULL;
}

struct libbex_channel *bex_platform_get_channel_by_id(struct libbex_platform *pl, uint64_t id)
{
	struct libbex_channel *ch;
	struct libbex_iter itr;

	bex_reset_iter(&itr, BEX_ITER_FORWARD);

	while (bex_platform_next_channel(pl, &itr, &ch) == 0) {
		if (ch->id == id)
			return ch;
	}

	return NULL;
}

static int subscribed_callback(struct libbex_platform *pl, struct libbex_event *ev)
{
	struct libbex_channel *ch;
	struct libbex_array *ar;
	struct libbex_value *id;
	struct libbex_iter itr;
	int rc = -EINVAL;

	bex_reset_iter(&itr, BEX_ITER_FORWARD);
	while (bex_platform_next_channel(pl, &itr, &ch) == 0) {
		if (bex_channel_verify_event(ch, ev))
			break;
	}

	if (!ch) {
		DBG(EVENT, bex_debugobj(ev, "unknown subscribed event"));
		goto done;
	}

	ar = bex_event_get_replies(ev);
	if (!ar)
		goto done;

	id = bex_array_get(ar, "chanId");
	bex_channel_set_id(ch, bex_value_get_u64(id));

	bex_channel_set_subscribed(ch, 1);
	bex_channel_update_heartbeat(ch);
	rc = 0;
done:
	bex_event_reset_reply(ev);
	return rc;
}

int bex_platform_subscribe_channel(struct libbex_platform *pl, struct libbex_channel *ch)
{
	int rc = 0, tries = 0;

	if (!ch || !ch->subscribe || bex_channel_is_subscribed(ch))
		return -EINVAL;

	DBG(PLAT, bex_debugobj(pl, "subscribing channel %s [%p]", ch->name, ch));

	/* define reply */
	if (!bex_platform_get_event(pl, "subscribed")) {
		struct libbex_event *ev = bex_new_event("subscribed");

		bex_event_set_reply_callback(ev, subscribed_callback);
		bex_event_add_reply(ev, bex_new_value_str("channel", NULL));
		bex_event_add_reply(ev, bex_new_value_u64("chanId", 0));
		bex_platform_add_event(pl, ev);
		bex_unref_event(ev);
	}

	/* send request */
	rc = bex_platform_send_event(pl, ch->subscribe);
	if (rc)
		goto done;

	/* wait for reply */
	while (!rc && !bex_channel_is_subscribed(ch) && tries < 10) {
		rc = bex_platform_service(pl);
		tries++;
	}

done:
	return  rc ? rc :
		bex_channel_is_subscribed(ch) ? 0 : -EINVAL;
}

int bex_platform_subscribe_channels(struct libbex_platform *pl)
{
	struct libbex_channel *ch;
	struct libbex_iter itr;
	int rc = 0;

	if (!pl)
		return -EINVAL;

	bex_reset_iter(&itr, BEX_ITER_FORWARD);

	while (bex_platform_next_channel(pl, &itr, &ch) == 0) {
		if (bex_channel_is_subscribed(ch))
			continue;

		rc += bex_platform_subscribe_channel(pl, ch);
	}

	return rc;
}
