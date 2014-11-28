AC_PREREQ([2.68])
AC_INIT([tiksock], [0.8], [donwen9011@gmail.com])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
PKG_PROG_PKG_CONFIG

# Remember externally set CFLAGS
EXTERNAL_CFLAGS="$CFLAGS"

# Checks for programs. These may set default variables, such as CFLAGS
AC_PROG_CPP
AC_PROG_CC_C99
AC_PROG_RANLIB

# Reset the externally set CFLAGS after calling AC_PROG*
CFLAGS="$EXTERNAL_CFLAGS"

# Use the C language and compiler for the following checks
AC_LANG([C])

# Checks for libraries.

AC_CHECK_HEADERS([ev.h])
AC_CHECK_LIB(ev, ev_io_start,, [AC_MSG_ERROR([libev not found])])


#PKG_CHECK_MODULES([libev], [libev], [have_libev=yes], [have_libev=no])
#AM_CONDITIONAL([LIB_libev],  [test "$have_libev" = "yes"])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdio.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Add debug support
AC_ARG_ENABLE(debug,
  AS_HELP_STRING(
    [--enable-debug],
    [enable debugging, default: no]),
    [case "${enableval}" in
      yes) debug=true ;;
      no)  debug=false ;;
      *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
    esac],
    [debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")
AM_COND_IF(DEBUG,
    AC_DEFINE(DEBUG, 1, [Define to 0 if this is a release build]),
    AC_DEFINE(DEBUG, 0, [Define to 1 or higher if this is a debug build]))


# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT


