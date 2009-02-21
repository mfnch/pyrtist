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

boxtopincdir=@boxincdir@
boxlibdir=@boxlibdir@
boxincdir=$(boxtopincdir)/box
boxpkgdir=@boxpkgdir@
ver_major=@BOX_VER_MAJOR@
ver_minor=@BOX_VER_MINOR@
boxcorever=$(ver_major).$(ver_minor)

BUILT_SOURCES = parser.h

INCLUDES = -I$(top_srcdir)

AM_YFLAGS = -d

AM_CFLAGS = -Wall

AM_CPPFLAGS = $(INCLTDL) \
              '-DBUILTIN_LIBRARY_PATH="$(boxlibdir)"' \
              '-DBUILTIN_PKG_PATH="$(boxpkgdir)"' \
              '-DC_LIBRARY_PATH="$(libdir)"' \
              '-DC_INCLUDE_PATH="$(boxtopincdir)"' \
              '-DBOXCORE_NAME="boxcore'$(boxcorever)'"'
# -DDEBUG_ARRAY -DDEBUG_EXEC

bin_PROGRAMS = box

# *** libboxcore ***
lib_LTLIBRARIES = libboxcore0.1.la
boxinc_HEADERS = types.h error.h mem.h array.h occupation.h \
  msgbase.h messages.h str.h print.h hashfunc.h hashtable.h \
  list.h
libboxcore0_1_la_SOURCES = types.h error.c mem.c mem.h array.c array.h \
  occupation.c occupation.h str.c print.c msgbase.c messages.c \
  hashfunc.c hashfunc.h hashtable.c hashtable.h list.c list.h
libboxcore0_1_la_LDFLAGS = -version-info 0:0:0

# *** box compiler ***
noinst_HEADERS = defaults.h \
  fileutils.h registers.h virtmach.h \
  tokenizer.h parserh.h typesys.h \
  expr.h compiler.h box.h builtins.h bltinstr.h bltinio.h \
  vmalloc.h vmproc.h vmsym.h \
  vmsymstuff.h paths.h container.h autogen.h

box_SOURCES = \
  fileutils.c virtmach.c vmalloc.c vmexec.c vmproc.c vmsym.c \
  symbols.c tokenizer.l parser.y registers.c expr.c \
  typesys.c compiler.c box.c builtins.c bltinstr.c bltinio.c vmsymstuff.c \
  paths.c main.c container.c autogen.c

box_LDADD = $(LIBLTDL) libboxcore0.1.la
box_LDFLAGS = -static
box_DEPENDENCIES=$(LTDLDEPS)

EXTRA_DIST = structure.c

install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
	  mv -f box$(EXEEXT) box$(ver_major).$(ver_minor)$(EXEEXT) && \
	  $(LN_S) box$(ver_major).$(ver_minor)$(EXEEXT) box$(EXEEXT)

uninstall-hook:
	cd $(DESTDIR)$(bindir) && \
	  rm -f box$(ver_major).$(ver_minor)$(EXEEXT)
