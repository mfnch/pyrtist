# set the include path found by configure
INCLUDES = $(all_includes)

BUILT_SOURCES = parser.h

AM_YFLAGS = -d

bin_PROGRAMS = box

noinst_HEADERS = defaults.h types.h str.h messages.h debug.h \
  array.h collection.h registers.h virtmach.h \
  tokenizer.h parserh.h compiler.h builtins.h

box_SOURCES = str.c messages.c array.c collection.c virtmach.c vmexec.c \
  symbols.c tokenizer.l parser.y registers.c typeman.c compiler.c \
  builtins.c main.c

EXTRA_DIST = structure.c
