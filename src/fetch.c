
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <libwebsockets.h>

#include <libbex.h>

#include "c.h"
#include "nls.h"
#include "xalloc.h"

static void __attribute__((__noreturn__)) usage(void)
{
	fputs(USAGE_HEADER, stdout);
	printf(_(" %s [options] [command]\n"), program_invocation_short_name);

	fputs(USAGE_SEPARATOR, stdout);
	fputs(_("Fetch data from platform.\n"), stdout);

	fputs(USAGE_OPTIONS, stdout);
	fputs(_(" -V, --version              print version\n"), stdout);
	fputs(_(" -h, --help                 this help\n"), stdout);

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	int c;

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

	return EXIT_SUCCESS;
}


