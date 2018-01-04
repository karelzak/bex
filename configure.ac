AC_INIT([bex],
	[0.1],
	[kzak@redhat.com],,
	[bex])


AC_PREREQ([2.60])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall foreign 1.10 tar-pax dist-bzip2 no-dist-gzip dist-xz -Wno-portability subdir-objects])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])],
			    [AC_SUBST([AM_DEFAULT_VERBOSITY], [1])])

AC_CONFIG_SRCDIR([fetch.c])
AC_PREFIX_DEFAULT([/usr])

AM_PROG_CC_C_O
AC_PROG_MKDIR_P
AC_PROG_CC_STDC
AC_CANONICAL_HOST
AC_C_CONST
AC_C_VOLATILE
AC_C_BIGENDIAN

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

AC_SYS_LARGEFILE
AC_HEADER_ASSERT

AC_CONFIG_HEADERS([config.h])

m4_ifndef([PKG_PROG_PKG_CONFIG],
  [m4_fatal([Could not locate the pkg-config autoconf
    macros. These are usually located in /usr/share/aclocal/pkg.m4.
    If your macros are in a different location, try setting the
    environment variable AL_OPTS="-I/other/macro/dir" before running
    ./autogen.sh or autoreconf again.])])
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([WEBSOCKETS], [libwebsockets])


#
# Don't use configure.ac to replace install paths! See Makefile PATHFILES for
# more details.
#
AC_CONFIG_FILES([
Makefile
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
