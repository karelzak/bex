
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
#include "colors.h"

static int count;
static int tu = 1, te = 1;
static long double last_price = 0;

/*
 * te: fast messages -- as soon as they match in the trading engine, but without ID (use SEQ ID as ID)
 * tu: delay 1-2 seconds in traiding engine, but have trade ID
 */
static int trades_callback(struct libbex_platform *pl, struct libbex_channel *ch)
{
	const char *type = bex_channel_get_reply_type(ch);
	struct libbex_array *ar;
	struct libbex_value *am, *pr;
	const struct libbex_symbol *sy;
	long double price;

	if (type && tu + te < 2) {
		if (tu == 0 && endswith(type, "tu"))
			return 0;
		if (te == 0 && endswith(type, "te"))
			return 0;
	} else if (!type) {
		if (!tu || !te)
			return 0;
		type = "";
	}

	sy = bex_channel_get_symbol(ch);

	ar = bex_channel_get_replies(ch);
	am = bex_array_get(ar, "AMOUNT");
	pr = bex_array_get(ar, "PRICE");

	price = bex_value_get_float(pr);

	if (last_price) {
		if (last_price < price)
			color_scheme_enable("price-up", UL_COLOR_GREEN);
		else if (last_price > price)
			color_scheme_enable("price-down", UL_COLOR_BOLD_RED);
	}
	last_price = price;

	if (!sy) {
	       fprintf(stdout, "%s: %.2Lf : %+.8Lf\n",
                       bex_channel_get_symbolname(ch),
                       price,
                       bex_value_get_float(am));
	       return 0;
	}

	fprintf(stdout, "%s/%s: ",
			bex_symbol_get_leftname(sy),
			bex_symbol_get_rightname(sy));

	fprintf(stdout, bex_symbol_get_price_format(sy),
			price);

	fputc(' ', stdout);

	fprintf(stdout, bex_symbol_get_amount_format(sy),
			bex_value_get_float(am));

	color_disable();

	fputc('\n', stdout);
	return 0;
}

static void __attribute__((__noreturn__)) usage(void)
{
	fputs(USAGE_HEADER, stdout);
	printf(_(" %s [options] <pair> [...]\n"), program_invocation_short_name);

	fputs(USAGE_SEPARATOR, stdout);
	fputs(_("Platform trades.\n"), stdout);

	fputs(USAGE_OPTIONS, stdout);
	fputs(_(" -V, --version              print version\n"), stdout);
	fputs(_(" -h, --help                 this help\n"), stdout);

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int c, count_max = 0;
	int colormode = UL_COLORMODE_AUTO;
	const char *uri = LIBBEX_DEFAULT_URI;
	struct libbex_platform *pl;
	static const struct option longopts[] = {
		{ "help",	no_argument,		0, 'h' },
		{ "version",	no_argument,		0, 'V' },
		{ "color",      optional_argument,	0, 'L' },
		{ "count",	required_argument,	0, 'c' },
		{ "ignore-tu",	no_argument,		0, 'u' },
		{ "ignore-te",	no_argument,		0, 'e' },
		{ NULL, 0, 0, 0 },
	};

	while ((c = getopt_long(argc, argv, "c:hVue", longopts, NULL)) != -1) {

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
		case 'u':
			tu = 0;
			break;
		case 'e':
			te = 0;
			break;
		case 'L':
			if (optarg)
				colormode = colormode_or_err(optarg,
						_("unsupported color mode"));
			break;
		default:
			errtryhelp(1);
		}
	}

	colors_init(colormode, "bex-trades");
	bex_init_debug(0);

	pl = bex_new_platform(uri);
	if (!pl)
		err(EXIT_FAILURE, _("failed to create platform instance for %s"), uri);

	while (optind < argc) {
		struct libbex_channel *ch = bex_new_trades_channel(argv[optind]);

		if (!ch)
			goto done;

		bex_channel_set_reply_callback(ch, trades_callback);
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


