
#include "bexP.h"

static int is_trades_event(struct libbex_channel *ch, struct libbex_event *ev)
{
	struct libbex_array *ar = bex_event_get_replies(ev);
	struct libbex_value *va;

	if (!ar)
		return 0;

	/* check channel family */
	va = bex_array_get(ar, "channel");
	if (!va || strcmp(bex_value_get_str(va), "trades") != 0)
		return 0;

	/* check symbol */
	va = bex_array_get(ar, "symbol");
	if (!va || strcmp(bex_value_get_str(va), bex_channel_get_symbolname(ch)) != 0)
		return 0;

	DBG(CHAN, bex_debugobj(ch, "trades event detected"));
	return 1;
}

struct libbex_channel *bex_new_trades_channel(const char *symbol)
{
	struct libbex_channel *ch = NULL;
	struct libbex_event *ev;
	char name[64];

	if (!symbol)
		return NULL;

	/* subscribe event definition */
	ev = bex_new_event("subscribe");
	if (!ev)
		goto err;

	bex_event_add_value(ev, bex_new_value_str("channel", "trades"));
	bex_event_add_value(ev, bex_new_value_str("symbol", symbol));

	snprintf(name, sizeof(name), "trades:%s", symbol);
	ch = bex_new_channel(name);
	if (!ch)
		goto err;

	bex_channel_set_subscribe_event(ch, ev);
	bex_unref_event(ev);
	ev = NULL;

	bex_channel_set_verify_callback(ch, is_trades_event);
	bex_channel_set_symbolname(ch, symbol);

	/* reply definition */
	bex_channel_add_reply(ch, bex_new_value_u64("ID", 0));
	bex_channel_add_reply(ch, bex_new_value_u64("MTS", 0));
	bex_channel_add_reply(ch, bex_new_value_float("AMOUNT", 0));
	bex_channel_add_reply(ch, bex_new_value_float("PRICE", 0));

	return ch;
err:
	bex_unref_event(ev);
	bex_unref_channel(ch);
	return NULL;
}
