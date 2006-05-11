AC_INIT(box, 0.1, fnch@libero.it)

AC_PREREQ(2.52)
AC_CONFIG_SRCDIR([src/compiler.c])
AC_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_LEX
AC_PROG_YACC

# Checks for libraries.
AC_CHECK_LIB([m], [floor])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([floor memmove memset pow rint sqrt strdup strncasecmp strndup])

AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])
AC_OUTPUT
