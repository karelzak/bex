/*
 * Copyright (C) 2018 Karel Zak <karel.zak.007@gmail.com>
 *
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 */

/**
 * SECTION: init
 * @title: Library initialization
 * @short_description: initialize debugging
 */

#include <stdarg.h>

#include "bexP.h"

BEX_DEBUG_DEFINE_MASK(libbex);
BEX_DEBUG_DEFINE_MASKNAMES(libbex) =
{
	{ "all",  BEX_DEBUG_ALL,	"info about all subsystems" },
	{ "help", BEX_DEBUG_HELP,	"this help" },
	{ NULL, 0 }
};

/**
 * bex_init_debug:
 * @mask: debug mask (0xffff to enable full debugging)
 *
 * If the @mask is not specified, then this function reads
 * the LIBBEX_DEBUG environment variable to get the mask.
 *
 * Already initialized debugging stuff cannot be changed. Calling
 * this function twice has no effect.
 */
void bex_init_debug(int mask)
{
	if (libbex_debug_mask)
		return;

	__BEX_INIT_DEBUG(libbex, BEX_DEBUG_, mask, LIBBEX_DEBUG);

	if (libbex_debug_mask != BEX_DEBUG_INIT
	    && libbex_debug_mask != (BEX_DEBUG_HELP|BEX_DEBUG_INIT)) {
		const char *ver = NULL;

		bex_get_library_version(&ver);

		DBG(INIT, bex_debug("library debug mask: 0x%04x", libbex_debug_mask));
		DBG(INIT, bex_debug("library version: %s", ver));
	}

	ON_DBG(HELP, bex_debug_print_masks("LIBBEX_DEBUG",
				BEX_DEBUG_MASKNAMES(libbex)));
}
