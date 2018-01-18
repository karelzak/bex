
#include "bexP.h"

struct libbex_channel *bex_new_ticker_channel(const char *pair)
{
	struct libbex_channel *ch = NULL;
	struct libbex_event *ev;

	if (!pair)
		return NULL;

	/* subscribe event definition */
	ev = bex_new_event("subscribe");
	if (!ev)
		goto err;

	bex_event_add_value(ev, bex_new_value_str("channel", "ticker"));
	bex_event_add_value(ev, bex_new_value_str("symbol", pair));

	ch = bex_new_channel(pair);
	if (!ch)
		goto err;

	bex_channel_set_subscribe_event(ch, ev);
	bex_unref_event(ev);
	ev = NULL;

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
