/*
 * version.c - Return the version of the libmount library
 *
 * Copyright (C) 2008-2016 Karel Zak <kzak@redhat.com>
 *
 */

/**
 * SECTION: version-utils
 * @title: Version functions
 * @short_description: functions to get the library version.
 */

#include <ctype.h>

#include "bexP.h"

static const char *lib_version = LIBBEX_VERSION;

/**
 * bex_parse_version_string:
 * @ver_string: version string (e.g "2.18.0")
 *
 * Returns: release version code.
 */
int bex_parse_version_string(const char *ver_string)
{
	const char *cp;
	int version = 0;

	assert(ver_string);

	for (cp = ver_string; *cp; cp++) {
		if (*cp == '.')
			continue;
		if (!isdigit(*cp))
			break;
		version = (version * 10) + (*cp - '0');
	}
	return version;
}

/**
 * bex_get_library_version:
 * @ver_string: return pointer to the static library version string if not NULL
 *
 * Returns: release version number.
 */
int bex_get_library_version(const char **ver_string)
{
	if (ver_string)
		*ver_string = lib_version;

	return bex_parse_version_string(lib_version);
}
