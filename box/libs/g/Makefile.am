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

boxlibdir=@boxlibdir@
boxincdir=@boxincdir@

boxlib_LTLIBRARIES = libg.la

dist_boxinc_DATA = g.bxh color.bxh window.bxh

AM_CPPFLAGS = -I../../src
libg_la_SOURCES = g.c bm1bit.c bm4bit.c bm8bit.c bmcommon.c fig.c graphic.c \
                  grpline.c ps.c rasterizer.c buffer.c error.c i_window.c

libg_la_LDFLAGS = -module
