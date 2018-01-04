AC_INIT([bex],
	[0.1],
	[kzak@redhat.com],,
	[bex])


AC_PREREQ([2.60])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([-Wall foreign 1.10 tar-pax dist-bzip2 no-dist-gzip dist-xz -Wno-portability subdir-objects])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])],
			    [AC_SUBST([AM_DEFAULT_VERBOSITY], [1])])

AC_CONFIG_SRCDIR([src/fetch.c])
AC_PREFIX_DEFAULT([/usr])

AM_PROG_CC_C_O
AC_PROG_MKDIR_P
AC_PROG_CC_STDC
AC_CANONICAL_HOST
AC_C_CONST
AC_C_VOLATILE
AC_C_BIGENDIAN
AC_SYS_LARGEFILE
AC_HEADER_ASSERT

dnl Compiler warnings
UL_WARN_ADD([-fno-common])
UL_WARN_ADD([-Wall])
UL_WARN_ADD([-Werror=sequence-point])
UL_WARN_ADD([-Wextra])
UL_WARN_ADD([-Wextra-semi])
UL_WARN_ADD([-Wembedded-directive])
UL_WARN_ADD([-Wmissing-declarations])
UL_WARN_ADD([-Wmissing-parameter-type])
UL_WARN_ADD([-Wmissing-prototypes])
UL_WARN_ADD([-Wno-missing-field-initializers])
UL_WARN_ADD([-Wredundant-decls])
UL_WARN_ADD([-Wsign-compare])
UL_WARN_ADD([-Wtype-limits])
UL_WARN_ADD([-Wuninitialized])
UL_WARN_ADD([-Wunused-but-set-parameter])
UL_WARN_ADD([-Wunused-but-set-variable])
UL_WARN_ADD([-Wunused-parameter])
UL_WARN_ADD([-Wunused-result])
UL_WARN_ADD([-Wunused-variable])
UL_WARN_ADD([-Wnested-externs])
UL_WARN_ADD([-Wpointer-arith])
UL_WARN_ADD([-Wstrict-prototypes])
UL_WARN_ADD([-Wformat-security])
UL_WARN_ADD([-Wimplicit-function-declaration])
AC_SUBST([WARN_CFLAGS])

UL_WARN_ADD([-Wno-clobbered], [BSD_WARN_CFLAGS])
AC_SUBST([BSD_WARN_CFLAGS])
UL_WARN_ADD([-Wno-unused-parameter], [NO_UNUSED_WARN_CFLAGS])
AC_SUBST([NO_UNUSED_WARN_CFLAGS])


dnl version details from <major>.<minor>[-<suffix>]
PACKAGE_VERSION_MAJOR=$(echo $PACKAGE_VERSION | awk -F. '{print $1}')
PACKAGE_VERSION_MINOR=$(echo $PACKAGE_VERSION | awk -F. '{print $2}' \
					      | awk -F- '{print $1}')

PACKAGE_VERSION_RELEASE=$(echo $PACKAGE_VERSION | awk -F. '{print $3}' \
						| sed 's/.*@<:@^@<:@:digit:@:>@@:>@.*/0/')

dnl libbex version
LIBBEX_VERSION="$PACKAGE_VERSION_MAJOR.$PACKAGE_VERSION_MINOR.$PACKAGE_VERSION_RELEASE"
LIBBEX_LT_MAJOR=1
LIBBEX_LT_MINOR=1
LIBBEX_LT_MICRO=0
LIBBEX_VERSION_INFO=`expr $LIBBEX_LT_MAJOR + $LIBBEX_LT_MINOR`:$LIBBEX_LT_MICRO:$LIBBEX_LT_MINOR

linux_os=no
bsd_os=no
AS_CASE([${host_os}],
  [*linux*],
     [linux_os=yes],
  [*darwin*],
     [darwin_os=yes],
  [*bsd*],
     [bsd_os=yes])
AM_CONDITIONAL([LINUX], [test "x$linux_os" = xyes])
AM_CONDITIONAL([DARWIN], [test "x$darwin_os" = xyes])
AM_CONDITIONAL([BSD], [test "x$bsd_os" = xyes])

AS_IF([test  "x$darwin_os" = xyes], [
  AC_DEFINE([_DARWIN_C_SOURCE], [1], [Enable MAP_ANON in sys/mman.h on Mac OS X])
])


dnl libtool-2       
LT_INIT             
                    
dnl check supported linker flags
AX_CHECK_VSCRIPT    

m4_ifndef([PKG_PROG_PKG_CONFIG],
  [m4_fatal([Could not locate the pkg-config autoconf
    macros. These are usually located in /usr/share/aclocal/pkg.m4.
    If your macros are in a different location, try setting the
    environment variable AL_OPTS="-I/other/macro/dir" before running
    ./autogen.sh or autoreconf again.])])
PKG_PROG_PKG_CONFIG

dnl Don't forget to maintain alternatively allowed versions in autogen.sh!
AM_GNU_GETTEXT_VERSION([0.18.3])
AM_GNU_GETTEXT([external])

AS_IF([test -d "$srcdir/po"], [
  ALL_LINGUAS=`cd $srcdir/po > /dev/null && ls *.po | sed 's/\.po//g'`
], [
  ALL_LINGUAS="af am ar as be bg bn_IN bn ca cs cy da de el en_GB es et eu_ES fa fi fr gl gu he hi hr hu hy id is it ja ka kn ko ku lo lt lv mk ml mr ms my nb nl nn no nso or pa pl pt_BR pt ro ru si sk sl sq sr@Latn sr sv ta te th tr uk ur vi zh_CN zh_TW zu"
])

AC_CHECK_HEADERS([linux/compiler.h linux/blkpg.h linux/major.h], [], [], [
#ifdef HAVE_LINUX_COMPILER_H
#include <linux/compiler.h>
#endif
])
AC_CHECK_HEADERS([ \
	asm/io.h \
	err.h \
	errno.h \
	fcntl.h \
	getopt.h \
	inttypes.h \
	linux/version.h \
	locale.h \
	netinet/in.h \
	paths.h \
	security/pam_appl.h \
	stdint.h \
	stdlib.h \
	endian.h \
	byteswap.h \
	sys/endian.h \
	sys/disk.h \
	sys/file.h \
	sys/io.h \
	sys/signalfd.h \
	sys/stat.h \
	sys/sysmacros.h \
	sys/time.h \
	sys/timex.h \
	sys/types.h \
	unistd.h
])

AC_CHECK_HEADERS([langinfo.h],
		[AM_CONDITIONAL([HAVE_LANGINFO], [true])],
		[AM_CONDITIONAL([HAVE_LANGINFO], [false])])

AC_CHECK_FUNCS([ \
	err \
	errx \
	fsync \
	utimensat \
	getdtablesize \
	getexecname \
	getrlimit \
	getsgnam \
	mempcpy \
	mkostemp \
	nanosleep \
	rpmatch \
	scandirat \
	strnchr \
	strndup \
	strnlen \
	warn \
	warnx \
])
AC_FUNC_FSEEKO


AC_MSG_CHECKING([whether program_invocation_short_name is defined])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
	#include <errno.h>
]], [[
	program_invocation_short_name = "test";
]])], [
	AC_MSG_RESULT([yes])
	AC_DEFINE([HAVE_PROGRAM_INVOCATION_SHORT_NAME], [1],
		[Define if program_invocation_short_name is defined])
], [
	AC_MSG_RESULT([no])
])


AC_MSG_CHECKING([whether __progname is defined])
AC_LINK_IFELSE([AC_LANG_PROGRAM([extern char *__progname;],
	   [if (*__progname == 0) return;])],
	   AC_DEFINE([HAVE___PROGNAME], [1], [Define if __progname is defined])
	   AC_MSG_RESULT([yes]),
	   AC_MSG_RESULT([no]))


PKG_CHECK_MODULES([WEBSOCKETS], [libwebsockets])

AC_SUBST([LIBBEX_VERSION])
AC_SUBST([LIBBEX_MAJOR_VERSION], $PACKAGE_VERSION_MAJOR)
AC_SUBST([LIBBEX_MINOR_VERSION], $PACKAGE_VERSION_MINOR)
AC_SUBST([LIBBEX_PATCH_VERSION], $PACKAGE_VERSION_RELEASE)
AC_SUBST([LIBBEX_VERSION_INFO])
AC_DEFINE_UNQUOTED([LIBBEX_VERSION], ["$LIBBEX_VERSION"], [libbex version string])

AC_CONFIG_HEADERS([config.h])

#
# Don't use configure.ac to replace install paths! See Makefile PATHFILES for
# more details.
#
AC_CONFIG_FILES([
Makefile
libbex/src/libbex.h
po/Makefile.in
])

AC_OUTPUT

AC_MSG_RESULT([
	${PACKAGE}  ${VERSION}

	compiler:          ${CC}
	cflags:            ${CFLAGS}
	ldflags:           ${LDFLAGS}

	warnings:

${WARN_CFLAGS}

	Type 'make' or 'make <utilname>' to compile.
])
