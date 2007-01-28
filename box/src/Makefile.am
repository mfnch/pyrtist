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

# set the include path found by configure
INCLUDES = $(all_includes)

BUILT_SOURCES = parser.h

AM_YFLAGS = -d

AM_CFLAGS = -Wall

bin_PROGRAMS = box

noinst_HEADERS = defaults.h types.h str.h print.h \
  msgbase.h messages.h debug.h \
  array.h collection.h registers.h virtmach.h \
  tokenizer.h parserh.h compiler.h builtins.h \
  hashfunc.h hashtable.h vmproc.h vmsym.h vmcommon.h \
  vmsymstuff.h

box_SOURCES = str.c print.c msgbase.c messages.c array.c collection.c \
  virtmach.c vmexec.c vmproc.c vmsym.c \
  symbols.c tokenizer.l parser.y registers.c typeman.c compiler.c \
  builtins.c main.c hashfunc.c hashtable.c vmsymstuff.c

EXTRA_DIST = structure.c
