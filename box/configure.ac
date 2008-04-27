############################################################################
#    Copyright (C) 2006 by Matteo Franchin                                 #
#    fnch@libero.it                                                        #
#                                                                          #
#    This program is free software; you can redistribute it and/or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

# $Id$

AC_INIT(box, 0.1, fnch@libero.it)

AC_PREREQ(2.52)
AC_CONFIG_SRCDIR([src/compiler.c])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE

# Directories where to store specific box library files
boxdir="$libdir/box"
boxlibdir="$boxdir/lib"
boxincdir="$boxdir/include"

# Libtool support
AC_PROG_LIBTOOL

# Checks for programs.
AC_PROG_CC
AC_PROG_LEX
AC_PROG_YACC

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h dlfcn.h])
AC_CHECK_HEADERS([cairo.h])

# Checks for libraries.
AC_CHECK_LIB([m], [floor])
AC_CHECK_LIB([dl], [dlopen])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor memmove memset pow rint sqrt strdup strncasecmp strndup])
AC_CHECK_FUNCS([getenv stat])

AC_ARG_WITH([cairo],
            AC_HELP_STRING([--with-cairo],
                           [use the cairo 2D graphic library [[default=no]]]),
            [ac_cv_with_cairo=$withval], [ac_cv_with_cairo=no])

if test "$ac_cv_with_cairo" = yes; then
  PKG_CHECK_MODULES(CAIRO,
                    [cairo >= 0.9.0],
                    [HAVE_CAIRO=1],
                    [if test $pkg_failed = yes; then
    AC_MSG_WARN([Package requirements (cairo >= 0.9.0) were not met])
                     elif test $pkg_failed = untried; then
    AC_MSG_WARN([The pkg-config script could not be found or is too old.])
                     fi])

fi

AC_SUBST([boxlibdir])
AC_SUBST([boxincdir])
AC_CONFIG_FILES([Makefile
                 src/Makefile
                 libs/Makefile
                 libs/g/Makefile
                 tests/Makefile
                 examples/Makefile
                 examples/smileys/Makefile])
AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([Configuration summary:])
AC_MSG_RESULT([----------------------])
AC_MSG_RESULT([])

if test "$ac_cv_with_cairo" = yes; then
  AC_MSG_RESULT([Support for the Cairo 2D graphic library: yes])
else
  AC_MSG_RESULT([Support for the Cairo 2D graphic library: no])
fi
