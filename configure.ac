define([tg_version], m4_normalize(m4_include([version])))
AC_CONFIG_MACRO_DIRS([m4])

AC_INIT([tg-expert], [tg_version], [], [tg-expert], [https://github.com/jefebus/tg-expert])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])

AC_PROG_CC
AC_CHECK_LIB([pthread], [pthread_mutex_init], [], [AC_MSG_ERROR([pthread not found])])
AC_CHECK_LIB([m], [sqrt], [], [AC_MSG_ERROR([libm not found])])
PKG_CHECK_MODULES([GTK], [gtk+-3.0 glib-2.0])
PKG_CHECK_MODULES([PORTAUDIO], [portaudio-2.0])
PKG_CHECK_MODULES([FFTW], [fftw3f])
# The PDF report (TG-16) is written with Cairo's PDF backend and PangoCairo.
# Both come with GTK 3; checked here so that a Cairo without PDF is reported.
PKG_CHECK_MODULES([CAIRO_PDF], [cairo-pdf pangocairo])

AC_CHECK_TOOL([WINDRES], [windres])
AC_PATH_PROG([TYPST], [typst])
AC_PATH_PROG([MSGFMT], [msgfmt])
AC_PATH_PROG([MSGMERGE], [msgmerge])
AC_PATH_PROG([XGETTEXT], [xgettext])
AM_CONDITIONAL([HAVE_MSGFMT], [test x$MSGFMT != x])
AS_IF([test x$MSGFMT = x], [AC_MSG_WARN([msgfmt not found: the program will be built without translations])])
AM_CONDITIONAL([HAVE_TYPST], [test x$TYPST != x])
AM_CONDITIONAL([HAVE_WINDRES], [test x$WINDRES != x])

AM_CONDITIONAL([BE_WINDOWS], [test x$OS = xWindows_NT])
AM_COND_IF([BE_WINDOWS], [AC_CONFIG_LINKS([icons/tg-timer.ico:icons/tg-timer.ico])])

CC_CHECK_LDFLAGS([-Wl,--as-needed], [AC_SUBST([AM_LDFLAGS], [-Wl,--as-needed])], [])
CC_CHECK_CFLAGS_APPEND([-Wall -Wextra], [], [])

AC_OUTPUT([Makefile icons/Makefile po/Makefile])

AC_MSG_RESULT([
        $PACKAGE_NAME $VERSION
        =====

        prefix:                 ${prefix}

        compiler:               ${CC}
        cflags:                 ${CFLAGS} ${AM_CFLAGS}
        ldflags:                ${LDFLAGS} ${AM_LDFLAGS}
])
