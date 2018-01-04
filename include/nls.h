#ifndef _BEX_NLS_H
#define _BEX_NLS_H

int main(int argc, char *argv[]);

#ifndef LOCALEDIR
#define LOCALEDIR "/usr/share/locale"
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#else
# undef setlocale
# define setlocale(Category, Locale) /* empty */
struct lconv
{
	char *decimal_point;
};
# undef localeconv
# define localeconv() NULL
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
# ifdef gettext_noop
#  define N_(String) gettext_noop (String)
# else
#  define N_(String) (String)
# endif
# define P_(Singular, Plural, n) ngettext (Singular, Plural, n)
#else
# undef bindtextdomain
# define bindtextdomain(Domain, Directory) /* empty */
# undef textdomain
# define textdomain(Domain) /* empty */
# define _(Text) (Text)
# define N_(Text) (Text)
# define P_(Singular, Plural, n) ((n) == 1 ? (Singular) : (Plural))
#endif

#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#else

typedef int nl_item;
extern char *langinfo_fallback(nl_item item);

# define nl_langinfo	langinfo_fallback

#endif /* !HAVE_LANGINFO_H */

#endif /* _BEX_NLS_H */
