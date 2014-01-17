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

BUILT_SOURCES = yparser.h

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
lib_LTLIBRARIES = libboxcore0.4.la
boxinc_HEADERS = version.h defaults.h types.h integers.h obj.h exception.h \
  callable.h core.h error.h mem.h stream.h array.h occupation.h srcpos.h   \
  srcmap.h msgbase.h messages.h strutils.h print.h hashfunc.h hashtable.h  \
  list.h vm_priv.h vmproc.h vmsym.h vmsymstuff.h vmdasm.h vm2txt.h vm2c.h  \
  vmserial.h vmstore.h bltinarray.h

libboxcore0_4_la_SOURCES = types.c types.h types_priv.h combs.c combs.h obj.c  \
  obj.h exception.c exception.h callable.c callable.h callable_priv.h core.c   \
  core.h coremath.c coremath.h core_priv.h error.c mem.c mem.h allocpool.c     \
  allocpool.h allocpool_priv.h array.c array.h stream.c stream.h stream_priv.h \
  filestream.c occupation.c occupation.h strutils.c print.c srcpos.c srcmap.c  \
  msgbase.c messages.c hashfunc.c hashfunc.h hashtable.c hashtable.h list.c    \
  list.h container.c container.h vm.c vm.h vm_priv.h vmdasm.c vmdasm.h         \
  vm2txt.c vm2txt.h vm2c.c vmserial.c vmserial.h vmstore.c vmstore.h vmexec.c  \
  vmexec_priv.h vmproc.c vmproc.h vmproc_priv.h vmsym.c vmsym.h vmsymstuff.c   \
  vmsymstuff.h vmopregs.c vmopregs_priv.h vmop.c vmop_priv.h str.c str.h       \
  bltinarray.c bltinarray_priv.h vmcode.c ast.c ast_priv.h ast.h value.c       \
  compiler.c operator.c namespace.c builtins.c bltinstr.c bltinio.c            \
  fileutils.c tokenizer.l macro.c yparser.y parser_priv.h parser.h registers.c \
  paths.c fileutils.h registers.h macro.h paths.h vmcode.h value.h             \
  compiler_priv.h compiler.h operator.h namespace.h builtins.h bltinstr.h      \
  bltinio.h

libboxcore0_4_la_LDFLAGS = -no-undefined -version-info 0:0:0
# ^^^ -no-undefined is needed for libtool to create shared libs on win32
# see http://article.gmane.org/gmane.comp.gnu.mingw.user/18727

libboxcore0_4_la_LIBADD = $(LIBLTDL)

dist_boxpkg_DATA=core.dox

# *** box compiler ***
noinst_HEADERS = vmdasm_priv.h

box_SOURCES = argparse.c argparse.h argparse_priv.h main.c

box_LDADD = libboxcore0.4.la
box_LDFLAGS = -static
box_DEPENDENCIES=$(LTDLDEPS) libboxcore0.4.la

# EXTRA_DIST=

install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
	  mv -f box$(EXEEXT) box$(ver_major).$(ver_minor)$(EXEEXT) && \
	  $(LN_S) box$(ver_major).$(ver_minor)$(EXEEXT) box$(EXEEXT)

uninstall-hook:
	cd $(DESTDIR)$(bindir) && \
	  rm -f box$(ver_major).$(ver_minor)$(EXEEXT)
