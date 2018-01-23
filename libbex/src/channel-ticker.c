
#include "bexP.h"

static int is_ticker_event(struct libbex_channel *ch, struct libbex_event *ev)
{
	struct libbex_array *ar = bex_event_get_replies(ev);
	struct libbex_value *va;

	if (!ar)
		return 0;

	/* check channel family */
	va = bex_array_get(ar, "channel");
	if (!va || strcmp(bex_value_get_str(va), "ticker") != 0)
		return 0;

	/* check symbol */
	va = bex_array_get(ar, "symbol");
	if (!va || strcmp(bex_value_get_str(va), bex_channel_get_symbol(ch)) != 0)
		return 0;

	DBG(CHAN, bex_debugobj(ch, "ticker event detected"));
	return 1;
}

struct libbex_channel *bex_new_ticker_channel(const char *symbol)
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

	bex_event_add_value(ev, bex_new_value_str("channel", "ticker"));
	bex_event_add_value(ev, bex_new_value_str("symbol", symbol));

	snprintf(name, sizeof(name), "ticker:%s", symbol);
	ch = bex_new_channel(name);
	if (!ch)
		goto err;

	bex_channel_set_subscribe_event(ch, ev);
	bex_unref_event(ev);
	ev = NULL;

	bex_channel_set_verify_callback(ch, is_ticker_event);
	bex_channel_set_symbol(ch, symbol);

	/* reply definition */
	bex_channel_add_reply(ch, bex_new_value_float("FRR", 0));
	bex_channel_add_reply(ch, bex_new_value_float("BID", 0));
	bex_channel_add_reply(ch, bex_new_value_u64(  "BID_PERIOD", 0));
	bex_channel_add_reply(ch, bex_new_value_float("BID_SIZE", 0));
	bex_channel_add_reply(ch, bex_new_value_float("ASK", 0));
	bex_channel_add_reply(ch, bex_new_value_u64(  "ASK_PERIOD", 0));
	bex_channel_add_reply(ch, bex_new_value_float("ASK_SIZE", 0));
	bex_channel_add_reply(ch, bex_new_value_float("DAILY_CHANGE", 0));
	bex_channel_add_reply(ch, bex_new_value_float("DAILY_CHANGE_PERC", 0));
	bex_channel_add_reply(ch, bex_new_value_float("LAST_PRICE", 0));
	bex_channel_add_reply(ch, bex_new_value_float("VOLUME", 0));
	bex_channel_add_reply(ch, bex_new_value_float("HIGH", 0));
	bex_channel_add_reply(ch, bex_new_value_float("LOW", 0));

	return ch;
err:
	bex_unref_event(ev);
	bex_unref_channel(ch);
	return NULL;
}
