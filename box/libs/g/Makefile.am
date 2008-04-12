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

AM_CFLAGS = -Wall

boxlibdir=@boxlibdir@
boxincdir=@boxincdir@

boxlib_LTLIBRARIES = libg.la

dist_boxinc_DATA = g.bxh color.bxh window.bxh line.bxh circle.bxh poly.bxh \
                   put.bxh arrows.bxh electric.bxh pointlist.bxh text.bxh \
                   bounce.bxh

AM_CPPFLAGS = -I../../src
libg_la_SOURCES = g.c g.h objlist.c objlist.h pointlist.h \
                  bm1bit.c bm4bit.c bm8bit.c bitmap.h \
                  bmcommon.c bmcommon.h \
                  fig.c fig.h graphic.c graphic.h grpline.c \
                  ps.c eps.c bb.c bb.h \
                  rasterizer.c buffer.c buffer.h error.c error.h \
                  autoput.c autoput.h \
                  gpath.c gpath.h \
                  i_pointlist.c i_pointlist.h \
                  i_window.c i_window.h i_line.c i_line.h \
                  i_circle.c i_circle.h i_poly.c i_poly.h \
                  i_put.c i_put.h i_text.c i_text.h

libg_la_LDFLAGS = -module
