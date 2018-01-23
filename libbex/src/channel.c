
#include "bexP.h"
#include "strutils.h"

static void free_channel(struct libbex_channel *ch)
{
	if (!ch)
		return;

	DBG(CHAN, bex_debugobj(ch, "free [name=%s]", ch->name));
	bex_unref_event(ch->subscribe);
	bex_unref_array(ch->reply);
	free(ch->name);
	free(ch->symbol);

	DBG(CHAN, bex_debugobj(ch, "done"));
	free(ch);
}

/**
 * bex_new_channel:
 * @name: channel name
 *
 * The initial refcount is 1, and needs to be decremented to
 * release the resources of the filesystem.
 *
 * Returns: newly allocated struct libbex_channel.
 */
struct libbex_channel *bex_new_channel(const char *name)
{
	struct libbex_channel *ch = calloc(1, sizeof(*ch));
	if (!ch)
		goto err;

	DBG(CHAN, bex_debugobj(ch, "alloc [name=%s]", name));
	ch->refcount = 1;
	ch->name = strdup(name);
	if (!ch->name)
		goto err;
	INIT_LIST_HEAD(&ch->channels);
	return ch;
err:
	free_channel(ch);
	return NULL;
}

/**
 * bex_ref_channel:
 * @ch: channel pointer
 *
 * Increments reference counter.
 */
void bex_ref_channel(struct libbex_channel *ch)
{
	if (ch)
		ch->refcount++;
}

/**
 * bex_unref_channel:
 * @ch: channel pointer
 *
 * De-increments reference counter, on zero the @ch is automatically
 * deallocated.
 */
void bex_unref_channel(struct libbex_channel *ch)
{
	if (ch) {
		ch->refcount--;
		if (ch->refcount <= 0)
			free_channel(ch);
	}
}


/**
 * bex_channel_set_subscribe_event:
 * @ch: channel
 * @va: value
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_channel_set_subscribe_event(struct libbex_channel *ch, struct libbex_event *ev)
{
	if (!ch)
		return -EINVAL;

	bex_ref_event(ev);			/* new */
	bex_unref_event(ch->subscribe);		/* old */
	ch->subscribe = ev;

	DBG(CHAN, bex_debugobj(ch, "set %s subscribe event to %s [%p]", ch->name, ev->name, ev));
	return 0;
}

/**
 * bex_channel_get_replies
 * @ch: channel
 *
 * Returns: values list
 */
struct libbex_array *bex_channel_get_replies(struct libbex_channel *ch)
{
	if (!ch)
		return NULL;
	return ch->reply;
}

/**
 * bex_channel_set_reply_callback
 * @ch: channel
 * @fn: callback function
 *
 * Returns: 0 or <0 on error
 */
int bex_channel_set_reply_callback(struct libbex_channel *ch,
		int (*fn)(struct libbex_platform *, struct libbex_channel *))
{
	if (!ch)
		return -EINVAL;
	ch->callback = fn;
	return 0;
}

/**
 * bex_channel_set_verify_callback
 * @ch: channel
 * @fn: callback function
 *
 * Returns: 0 or <0 on error
 */
int bex_channel_set_verify_callback(struct libbex_channel *ch,
		int (*fn)(struct libbex_channel *, struct libbex_event *))
{
	if (!ch)
		return -EINVAL;
	ch->verify = fn;
	return 0;
}

int bex_channel_verify_event(struct libbex_channel *ch, struct libbex_event *ev)
{
	if (!ch || !ch->verify)
		return 0;

	return ch->verify(ch, ev);
}

/**
 * bex_channel_set_data
 * @ch: channel
 * @dt: data
 *
 * Returns: 0 or <0 on error
 */
int bex_channel_set_data(struct libbex_channel *ch, void *dt)
{
	if (!ch)
		return -EINVAL;
	ch->data = dt;
	return 0;
}

/**
 *
 * bex_channel_get_data
 * @ch: channel
 *
 * Returns: 0 or <0 on error
 */
void *bex_channel_get_data(struct libbex_channel *ch)
{
	if (!ch)
		return NULL;
	return ch->data;
}

/**
 * bex_channel_add_reply:
 * @ch: channel
 * @va: value
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_channel_add_reply(struct libbex_channel *ch, struct libbex_value *va)
{
	if (!va || !ch)
		return -EINVAL;

	DBG(CHAN, bex_debugobj(ch, "add reply %s [%p]", va->name, va));
	if (!ch->reply) {
		ch->reply = bex_new_array(3);
		if (!ch->reply)
			return -ENOMEM;
	}

	return bex_array_add(ch->reply, va);
}

/**
 * bex_channel_remove_reply:
 * @ch: channel
 * @val: value
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_channel_remove_reply(struct libbex_channel *ch, struct libbex_value *va)
{
	if (!va || !ch)
		return -EINVAL;
	if (!ch->reply)
		return 0;

	DBG(CHAN, bex_debugobj(ch, "remove reply %s [%p]", va->name, va));
	return bex_array_remove(ch->reply, va);
}

/**
 * bex_channel_update_reply:
 * @ch: channel
 * @str: unparsed data
 *
 * Parse @str and fill reply array.
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_channel_update_reply(struct libbex_channel *ch, const char *str)
{
	if (!ch || !str)
		return -EINVAL;

	return bex_array_fill_from_string(ch->reply, str);
}

/**
 * bex_channel_set_subscribed:
 * @ch: channel
 * @x: 0 or 1
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_channel_set_subscribed(struct libbex_channel *ch, int x)
{
	if (!ch)
		return -EINVAL;
	ch->subscribed = x;

	DBG(CHAN, bex_debugobj(ch, "change %s subscribed status to %s",
			ch->name, ch->subscribed ? "TRUE" : "FALSE"));
	return 0;
}

/**
 * bex_channel_is_subscribed:
 * @ch: channel
 *
 * Returns: 0 or 1
 */
int bex_channel_is_subscribed(struct libbex_channel *ch)
{
	return ch && ch->subscribed ? 1 : 0;
}

/**
 * bex_channel_set_id:
 * @ch: channel
 * @id: identifier
 *
 * Returns: 0 on success or negative number in case of error.
 */
int bex_channel_set_id(struct libbex_channel *ch, uint64_t id)
{
	if (!ch)
		return -EINVAL;
	ch->id = id;
	return 0;
}

int bex_channel_update_heartbeat(struct libbex_channel *ch)
{
	DBG(CHAN, bex_debugobj(ch, "update heartbeat"));
	gettimeofday(&ch->last_update, NULL);
	return 0;
}

const struct timeval *bex_channel_get_heartbeat(struct libbex_channel *ch)
{
	return &ch->last_update;
}

/**
 * bex_channel_set_symbol
 * @ch: channel
 * @sy: symbol
 *
 * Returns: 0 or <0 on error
 */
int bex_channel_set_symbol(struct libbex_channel *ch, const char *sy)
{
	char *p = NULL;

	if (!ch)
		return -EINVAL;

	if (sy) {
		p = strdup(sy);
		if (!p)
			return -ENOMEM;
	}

	free(ch->symbol);
	ch->symbol = p;
	return 0;
}

/**
 *
 * bex_channel_get_symbol
 * @ch: channel
 *
 * Returns: symbol string
 */
const char *bex_channel_get_symbol(struct libbex_channel *ch)
{
	return ch ? ch->symbol : NULL;
}


int bex_is_channel_string(const char *str, uint64_t *id)
{
	const char *p = (char *) str;
	char *end;

	p = skip_space(p);
	if (*p != '[')
		return 0;

	p = skip_space(++p);
	errno = 0;
	*id = strtoumax(p, &end, 10);
	if (errno || p == end)
		return 0;

	p = end;
	p = skip_space(p);
	if (*p != ',')
		return 0;

	return 1;
}

