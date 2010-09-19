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

ACLOCAL_AMFLAGS=-I libltdl/m4

boxlibdir=@boxlibdir@
boxpkgdir=@boxpkgdir@

AM_CFLAGS=-Wall @CAIRO_CFLAGS@
AM_CPPFLAGS=-I../../box -I../..
AM_LDFLAGS=-no-undefined -module -avoid-version

boxlib_LTLIBRARIES=libg.la

dist_boxpkg_DATA=g.bxh _color.bxh _style.bxh _gradient.bxh _window.bxh \
                 _line.bxh _circle.bxh _poly.bxh _put.bxh arrows.bxh \
                 _pointlist.bxh _text.bxh _matrix.bxh _bbox.bxh \
                 _rectangle.bxh \
                 label.bxh bounce.bxh electric.bxh

libg_la_SOURCES=g.c g.h objlist.c objlist.h pointlist.c pointlist.h \
                bm1bit.c bm4bit.c bm8bit.c bitmap.h \
                bmcommon.c bmcommon.h \
                fig.c fig.h graphic.c graphic.h hsv.c hsv.h \
                eps.c bb.c bb.h \
                rasterizer.c buffer.c buffer.h error.c error.h \
                autoput.c autoput.h \
                gpath.c gpath.h linetracer.c linetracer.h \
                formatter.c formatter.h psfonts.c psfonts.h \
                i_pointlist.c i_pointlist.h i_color.c \
                i_bbox.h i_gradient.c i_gradient.h \
                i_window.c i_window.h i_line.c i_line.h \
                i_circle.c i_circle.h i_poly.c i_poly.h \
                i_put.c i_put.h i_text.c i_text.h \
                i_style.c i_style.h

libg_la_LIBADD=@CAIRO_LIBS@ ../../box/libboxcore0.2.la

if BUILD_WINCAIRO
libg_la_SOURCES+=wincairo.c wincairo.h
endif

