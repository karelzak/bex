
dnl UL_NCURSES_CHECK(NAME)
dnl
dnl Initializes $have_<name>, NCURSES_LIBS and NCURSES_CFLAGS variables according to
dnl <name>{6,5}_config output.
dnl
dnl The expected <name> is ncurses or ncursesw.
dnl
AC_DEFUN([UL_NCURSES_CHECK], [
  m4_define([suffix], $1)
  m4_define([SUFFIX], m4_toupper($1))

  # ncurses6-config
  #
  AS_IF([test "x$have_[]suffix" = xno], [
    AC_CHECK_TOOL(SUFFIX[]6_CONFIG, suffix[]6-config)
    if AC_RUN_LOG([$SUFFIX[]6_CONFIG --version >/dev/null]); then
      have_[]suffix=yes
      NCURSES_LIBS=`$SUFFIX[]6_CONFIG --libs`
      NCURSES_CFLAGS=`$SUFFIX[]6_CONFIG --cflags`
    else
      have_[]suffix=no
    fi
  ])

  # ncurses5-config
  #
  AS_IF([test "x$have_[]suffix" = xno], [
    AC_CHECK_TOOL(SUFFIX[]5_CONFIG, suffix[]5-config)
    if AC_RUN_LOG([$SUFFIX[]5_CONFIG --version >/dev/null]); then
      have_[]suffix=yes
      NCURSES_LIBS=`$SUFFIX[]5_CONFIG --libs`
      NCURSES_CFLAGS=`$SUFFIX[]5_CONFIG --cflags`
    else
      have_[]suffix=no
    fi
  ])

  # pkg-config (not supported by ncurses upstream by default)
  #
  AS_IF([test "x$have_[]suffix" = xno], [
    PKG_CHECK_MODULES(SUFFIX, [$1], [
      have_[]suffix=yes
      NCURSES_LIBS=${SUFFIX[]_LIBS}
      NCURSES_CFLAGS=${SUFFIX[]_CFLAGS}
    ],[have_[]suffix=no])
  ])

  # classic autoconf way
  #
  AS_IF([test "x$have_[]suffix" = xno], [
    AC_CHECK_LIB([$1], [initscr], [have_[]suffix=yes], [have_[]suffix=no])
    AS_IF([test "x$have_[]suffix" = xyes], [NCURSES_LIBS="-l[]suffix"])
  ])
])

dnl
dnl UL_TINFO_CHECK(NAME)
dnl
dnl Initializes $have_<name>, TINFO_LIBS and TINFO_CFLAGS variables.
dnl
dnl The expected <name> is tinfow or tinfo.
dnl
AC_DEFUN([UL_TINFO_CHECK], [
  m4_define([suffix], $1)
  m4_define([SUFFIX], m4_toupper($1))

  PKG_CHECK_MODULES(SUFFIX, [$1], [
    dnl pkg-config success
    have_[]suffix=yes
    TINFO_LIBS=${SUFFIX[]_LIBS}
    TINFO_CFLAGS=${SUFFIX[]_CFLAGS}
  ],[
    dnl If pkg-config failed, fall back to classic searching.
    AC_CHECK_LIB([$1], [tgetent], [
       have_[]suffix=yes
       TINFO_LIBS="-l[]suffix"
       TINFO_CFLAGS=""])
  ])
])
