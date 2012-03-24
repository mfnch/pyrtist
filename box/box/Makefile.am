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
lib_LTLIBRARIES = libboxcore0.3.la
boxinc_HEADERS = version.h defaults.h types.h error.h mem.h \
  array.h occupation.h srcpos.h msgbase.h messages.h strutils.h print.h \
  hashfunc.h hashtable.h list.h virtmach.h vmalloc.h vmproc.h vmsym.h \
  vmsymstuff.h
libboxcore0_3_la_SOURCES = types.h error.c mem.c mem.h array.c array.h \
  occupation.c occupation.h strutils.c print.c srcpos.c msgbase.c messages.c \
  hashfunc.c hashfunc.h hashtable.c hashtable.h list.c list.h \
  container.c container.h virtmach.c vmptr.h virtmach.h vmalloc.c vmalloc.h \
  vmexec.c vmproc.c vmproc.h vmsym.c vmsym.h vmsymstuff.c vmsymstuff.h \
  str.c str.h vmx.c vmx.h

libboxcore0_3_la_LDFLAGS = -no-undefined -version-info 0:0:0
# ^^^ -no-undefined is needed for libtool to create shared libs on win32
# see http://article.gmane.org/gmane.comp.gnu.mingw.user/18727

libboxcore0_3_la_LIBADD = $(LIBLTDL)

dist_boxpkg_DATA=core.dox

# *** box compiler ***
noinst_HEADERS = \
  fileutils.h registers.h tokenizer.h macro.h parserh.h typesys.h  paths.h \
  cmpproc.h ast.h cmpptrs.h value.h compiler.h operator.h \
  namespace.h autogen.h tsdesc.h builtins.h bltinstr.h bltinio.h bltinarray.h

box_SOURCES = \
  fileutils.c tokenizer.l macro.c parser.y registers.c typesys.c \
  paths.c main.c cmpproc.c ast.c value.c \
  compiler.c operator.c namespace.c autogen.c tsdesc.c \
  builtins.c bltinstr.c bltinio.c bltinarray.c

box_LDADD = libboxcore0.3.la
box_LDFLAGS = -static
box_DEPENDENCIES=$(LTDLDEPS)

# EXTRA_DIST=

install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
	  mv -f box$(EXEEXT) box$(ver_major).$(ver_minor)$(EXEEXT) && \
	  $(LN_S) box$(ver_major).$(ver_minor)$(EXEEXT) box$(EXEEXT)

uninstall-hook:
	cd $(DESTDIR)$(bindir) && \
	  rm -f box$(ver_major).$(ver_minor)$(EXEEXT)
