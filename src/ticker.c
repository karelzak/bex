
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/time.h>

#include <libbex.h>

#include "c.h"
#include "nls.h"
#include "xalloc.h"
#include "strutils.h"

static int count;

static int ticker_callback(struct libbex_platform *pl, struct libbex_channel *ch)
{
	struct libbex_array *ar = bex_channel_get_replies(ch);
	struct libbex_value *lp = bex_array_get(ar, "LAST_PRICE");
	struct libbex_value *hi = bex_array_get(ar, "HIGH");
	struct libbex_value *lo = bex_array_get(ar, "LOW");
	struct libbex_value *vo = bex_array_get(ar, "VOLUME");
	struct libbex_value *dc_perc = bex_array_get(ar, "DAILY_CHANGE_PERC");


	fprintf(stderr, "%s: %.2Lf (%.1Lf%%) high=%.2Lf, low=%.2Lf, 24h_volume=%Lg\n",
			bex_channel_get_symbolname(ch),
			bex_value_get_float(lp),
			bex_value_get_float(dc_perc) * 100,
			bex_value_get_float(hi),
			bex_value_get_float(lo),
			bex_value_get_float(vo));
	return 0;
}

static void __attribute__((__noreturn__)) usage(void)
{
	fputs(USAGE_HEADER, stdout);
	printf(_(" %s [options] <pair> [...]\n"), program_invocation_short_name);

	fputs(USAGE_SEPARATOR, stdout);
	fputs(_("Platform ticker.\n"), stdout);

	fputs(USAGE_OPTIONS, stdout);
	fputs(_(" -V, --version              print version\n"), stdout);
	fputs(_(" -h, --help                 this help\n"), stdout);

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int c, count_max = 0;
	const char *uri = LIBBEX_DEFAULT_URI;
	struct libbex_platform *pl;
	static const struct option longopts[] = {
		{ "help",	no_argument,		0, 'h' },
		{ "version",	no_argument,		0, 'V' },
		{ "count",	required_argument,	0, 'c' },
		{ NULL, 0, 0, 0 },
	};

	while ((c = getopt_long(argc, argv, "c:hV", longopts, NULL)) != -1) {

		switch(c) {
		case 'c':
			count_max = strtos64_or_err(optarg, _("failed to parse --count argument"));
			break;
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

	while (optind < argc) {
		struct libbex_channel *ch = bex_new_ticker_channel(argv[optind]);

		if (!ch)
			goto done;

		bex_channel_set_reply_callback(ch, ticker_callback);
		bex_platform_add_channel(pl, ch);
		bex_unref_channel(ch);
		optind++;
	}

	bex_platform_connect(pl);
	bex_platform_set_timeout(pl, 1000);

	bex_platform_subscribe_channels(pl);

	while (1) {
		if (count_max && count >= count_max)
			break;
		bex_platform_service(pl);
		count++;
	}

	bex_platform_unsubscribe_channels(pl);
done:
	bex_unref_platform(pl);

	return EXIT_SUCCESS;
}


