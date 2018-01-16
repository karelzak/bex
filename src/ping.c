
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/time.h>

#include <libbex.h>

#include "c.h"
#include "nls.h"
#include "xalloc.h"

static uint64_t lastping = 0;

static int pong_callback(struct libbex_platform *pl, struct libbex_event *ev)
{
	struct libbex_array *ar = bex_event_get_replies(ev);
	struct libbex_value *ts = ar ? bex_array_get(ar, "ts") : NULL;

	if (ts) {
		uint64_t end = bex_value_get_u64(ts);

		printf("PING %s: %ju ms\n",
				bex_platform_get_address(pl),
				end - lastping);
	}

	lastping = 0;
	return 0;
}

static void __attribute__((__noreturn__)) usage(void)
{
	fputs(USAGE_HEADER, stdout);
	printf(_(" %s [options]\n"), program_invocation_short_name);

	fputs(USAGE_SEPARATOR, stdout);
	fputs(_("Ping platform.\n"), stdout);

	fputs(USAGE_OPTIONS, stdout);
	fputs(_(" -V, --version              print version\n"), stdout);
	fputs(_(" -h, --help                 this help\n"), stdout);

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int c, tries;
	const char *uri = LIBBEX_DEFAULT_URI;
	struct libbex_platform *pl;
	struct libbex_event *ping, *ev;
	struct libbex_value *start;
	uint64_t last = 0;

	static const struct option longopts[] = {
		{ "help",	no_argument,		0, 'h' },
		{ "version",	no_argument,		0, 'V' },
		{ NULL, 0, 0, 0 },
	};

	while ((c = getopt_long(argc, argv, "+hV", longopts, NULL)) != -1) {

		switch(c) {
		case 'v':
		case 'V':
			printf(BEX_VERSION "\n");
			break;
		case 'h':
			usage();
			break;
		default:
			errtryhelp(1);
		}
	}

	bex_init_debug(0);

	pl = bex_new_platform(uri);
	if (!pl)
		err(EXIT_FAILURE, _("failed to create platform instance for %s"), uri);

	ev = bex_new_event("pong");
	bex_event_set_reply_callback(ev, pong_callback);
	bex_event_add_reply(ev, bex_new_value_u64("ts", 0));
	bex_platform_add_event(pl, ev);
	bex_unref_event(ev);

	/* send only; don't have to be added to platform */
	ping = bex_new_event("ping");
	bex_event_add_value(ping, (start = bex_new_value_u64("cid", 0)));

	bex_platform_connect(pl);
	bex_platform_set_timeout(pl, 1000);

	for (tries = 0; tries < 10; ) {
		if (!lastping) {
			struct timeval tv;
			uint64_t now;

			gettimeofday(&tv, NULL);
			now = (tv.tv_sec * 1000) + (tv.tv_usec/1000);

			if (now > last + 1000) {
				bex_platform_send_event(pl, ping);
				lastping = last = now;
				tries++;
			}
		}
		while (lastping)
			bex_platform_service(pl);
	}

	bex_unref_event(ping);
	bex_unref_platform(pl);

	return EXIT_SUCCESS;
}


