############################################################################
#    Copyright (C) 2006 by Matteo Franchin                                 #
#    fnch@users.sourceforge.net                                            #
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

ACLOCAL_AMFLAGS=-I libltdl/m4

boxlibdir=@boxlibdir@
boxpkgdir=@boxpkgdir@
boxlibgdir=$(boxpkgdir)/g

AM_CFLAGS=-Wall @CAIRO_CFLAGS@
AM_CPPFLAGS=-I../../box -I../..
AM_LDFLAGS=-no-undefined -module -avoid-version

boxlib_LTLIBRARIES=libg.la

dist_boxlibg_DATA= obj.box const.box gcore.box matrix.box transform.box	   \
  option.box raw.box cmdstream.box points.box path.box fill.box stroke.box \
  style.box poly.box text.box texts.box circle.box circles.box line.box	   \
  lines.box gradcore.box gradient.box clip.box hot.box constraints.box	   \
  put.box window.box bbox.box g2.box init.box color.box oldstyle.box	   \
  _gradient.bxh pattern.box _line.bxh _circle.bxh _poly.bxh _put.bxh	   \
  _pointlist.bxh _text.bxh area.box axes.box thickline.box picture.box

dist_boxpkg_DATA= arrows.bxh caps.box _arrows.bxh math.bxh tools.bxh	\
  _sphere.bxh _rectangle.bxh label.bxh bounce.bxh electric.bxh

libg_la_SOURCES=g.c g.h objlist.c objlist.h pointlist.c pointlist.h matrix.c  \
  matrix.h winmap.c winmap.h bm1bit.c bm4bit.c bm8bit.c bitmap.h bmcommon.c   \
  bmcommon.h fig.c fig.h graphic.c graphic.h cmd.c cmd.h hsv.c hsv.h eps.c    \
  bb.c bb.h rasterizer.c buffer.c buffer.h error.c error.h autoput.c	      \
  autoput.h gpath.c gpath.h linetracer.c linetracer.h formatter.c formatter.h \
  psfonts.c psfonts.h i_pointlist.c i_pointlist.h i_color.c i_gradient.c      \
  i_gradient.h i_window.c i_window.h i_line.c i_line.h i_circle.c i_circle.h  \
  i_poly.c i_poly.h i_put.c i_put.h i_text.c i_text.h i_style.c i_style.h     \
  obj.c obj.h bridge.c bridge.h csimpleput.c constraints.c constraints.h      \
  transform.c transform.h

libg_la_LIBADD=@CAIRO_LIBS@ ../../box/libboxcore0.3.la

if BUILD_WINCAIRO
libg_la_SOURCES+=wincairo.c wincairo.h
endif

