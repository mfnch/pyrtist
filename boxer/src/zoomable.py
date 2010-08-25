# Copyright (C) 2008, 2009, 2010
#  by Matteo Franchin (fnch@users.sourceforge.net)
#
# This file is part of Boxer.
#
#   Boxer is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Boxer is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Boxer.  If not, see <http://www.gnu.org/licenses/>.

import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import gtk.gdk
import os, os.path
import time
import gobject

color_background = 0xffffff00


#scroll:
#  compute new view.
#  Is it different from old view?
#  no) ==> exit
#  yes) ==> update_view



def debug():
  import sys
  from IPython.Shell import IPShellEmbed
  calling_frame = sys._getframe(1)
  IPShellEmbed([])(local_ns  = calling_frame.f_locals,
                   global_ns = calling_frame.f_globals)


def debug_msg(msg):
  print msg







def sgn(x):
  return (x >= 0.0) - (x <= 0.0)

def get_pixbuf_size(buf):
  """If buf is a PixBuf returns its size as a tuple (width, height).
  If buf is None return (0, 0)."""
  return (Point(buf.get_width(), buf.get_height())
          if buf != None else Point(0, 0))

def central_segments_intersection(s1_width, s2_width):
  diff = (s2_width - s1_width)/2
  return (0, diff, s1_width) if diff >= 0 else (-diff, 0, s2_width)

class Point(object):
  def __init__(self, x=None, y=None):
    if y == None:
      self.x, self.y = x if x != None else (0.0, 0.0)
    else:
      self.x = x
      self.y = y

  def __getitem__(self, idx):
    if idx == 0:
      return self.x
    elif idx == 1:
      return self.y
    else:
      raise IndexError("Point index out of range")

  def __iter__(self):
    return self.get_xy().__iter__()

  def __repr__(self):
    return "Point(%s, %s)" % self.xy

  def __add__(self, p):
    return Point(self.x + p.x, self.y + p.y)

  def __sub__(self, p):
    return Point(self.x - p.x, self.y - p.y)

  def get_xy(self):
    return (self.x, self.y)

  def set_xy(self, xy):
    try:
      self.x, self.y = xy   # is iterable
    except:
      self.x = self.y = xy  # is not iterable
    return self

  xy = property(get_xy, set_xy)

  def trunc(self, xmin, xmax, ymin, ymax):
    self.x = min(xmax, max(xmin, self.x))
    self.y = min(ymax, max(ymin, self.y))

  def scale(self, scale_factor):
    sf = Point()
    sf.set_xy(scale_factor)
    self.x *= sf.x
    self.y *= sf.y

class Rectangle(object):
  def __init__(self, corner1, corner2):
    self.corner1 = corner1
    self.corner2 = corner2

  def __repr__(self):
    return "Rectangle(%s, %s)" % (self.corner1, self.corner2)

  def get_dx(self):
    return self.corner2.x - self.corner1.x

  def get_dy(self):
    return self.corner2.y - self.corner1.y

  def translate(self, dr):
    self.corner1 += dr
    self.corner2 += dr

  def new_scaled(self, scale_factor, scale_in=True):
    sx, sy = Point(scale_factor)
    if not scale_in:
      sx, sy = (1/sx, 1/sy)
    hc1x, hc1y = (0.5*self.corner1.x, 0.5*self.corner1.y)
    hc2x, hc2y = (0.5*self.corner2.x, 0.5*self.corner2.y)
    cx, cy = (hc1x + hc2x, hc1y + hc2y)
    wx, wy = ((hc2x - hc1x)*sx, (hc2y - hc1y)*sy)
    c1 = Point(cx - wx, cy - wy)
    c2 = Point(cx + wx, cy + wy)
    return Rectangle(c1, c2)

  def new_reshaped(self, new_shape, scale_factor=None):
    sf = Point(new_shape.x/abs(self.get_dx()),
               new_shape.y/abs(self.get_dy()))
    if scale_factor != None:
      sf.scale(scale_factor)
    return self.new_scaled(sf)

class View(Rectangle):
  """Class to map pixel coordinate to arbitrary coordinates."""

  def __init__(self, size_in_pixels=None, topleft_coord=None,
               botright_coord=None):
    Rectangle.__init__(self, topleft_coord, botright_coord)
    self.view_size = size_in_pixels
    if (size_in_pixels != None and topleft_coord != None
        and botright_coord != None):
      self.reset()

  def reset(self, size_in_pixels=None, topleft_coord=None,
            botright_coord=None):
    """Reset the object to a new view."""
    if topleft_coord is not None:
      self.corner1 = topleft_coord
    if botright_coord is not None:
      self.corner2 = botright_coord
    if size_in_pixels is not None:
      self.view_size = size_in_pixels
    x1, y1 = self.corner1
    x2, y2 = self.corner2
    psx, psy = self.view_size
    self.dpix_dcoord = Point((x2 - x1)/psx, (y2 - y1)/psy)
    self.dcoord_dpix = Point(1.0/self.dpix_dcoord[0],
                             1.0/self.dpix_dcoord[1])

  def show(self, topleft_coord, botright_coord, size_in_pixels=None):
    """Shows the whole given rectangle, but not more (or - at least -
    shows as little as possible the region outside the rectagle)."""
    if size_in_pixels is not None:
      self.view_size = size_in_pixels
    x1, y1 = topleft_coord
    x2, y2 = botright_coord
    dx = x2 - x1
    dy = y2 - y1
    pic_aspect = abs(float(dy)/dx)
    vx, vy = self.view_size
    view_aspect = abs(float(vy)/vx)

    if pic_aspect > view_aspect:
      # fill vertically
      tx = 0.5*(abs(dy*vx/vy) - abs(dx))*sgn(dx)
      self.reset(topleft_coord=Point(x1 - tx, y1),
                 botright_coord=Point(x2 + tx, y2))
    else:
      # fill horizontally
      ty = 0.5*(abs(dx*vy/vx) - abs(dy))*sgn(dy)
      self.reset(topleft_coord=Point(x1, y1 - ty),
                 botright_coord=Point(x2, y2 + ty))

  def pix_to_coord(self, pix):
    """Transform pixel coordinates to picture coordinates."""
    x1, y1 = self.corner1
    dx_dpx, dy_dpy = self.dpix_dcoord
    return (x1 + pix[0]*dx_dpx, y1 + pix[0]*dy_dpy)

  def coord_to_pix(self, coord):
    """Transform picture coordinates to pixel coordinates."""
    x1, y1 = self.corner1
    dpx_dx, dpy_dy = self.dcoord_dpix
    return ((coord[0] - x1)*dpx_dx, (coord[1] - y1)*dpy_dy)

  def valid_coord_to_pix(self, coord):
    """Transform picture coordinates to valid pixel coordinates
    (i.e. integer numbers within the boundaries of the picture)."""
    px, py = self.coord_to_pix(coord)
    return (int(max(0.0, min(self.view_size[0], px))),
            int(max(0.0, min(self.view_size[1], py))))

class ImageDrawer(object):
  def __init__(self):
    pass

  def update(self, pixbuf_output, pix_view, coord_view=None):
    """This function is called by ZoomableArea to update the view (redraw the
    picture). There are two possibilities:
     1. coord_view is not provided. The function knows how many pixel (in both
        x and y directions) are available and has to draw the picture in the
        best possible way in the available space (possibly, increasing the
        image size until the picture hits one of the boundaries);
     2. coord_view is provided. The function has to draw just the provided
        coordinate view of the picture, producing an output filling the whole
        pix_view.
    In both cases the total extent of the figure is returned as a Rectangle.
    """
    raise NotImplementedError("ImageDrawer.update is not implemented")

class ZoomableArea(gtk.DrawingArea):
  def __init__(self, drawer,
               hadjustment=None, vadjustment=None,
               buf_margin=0.25):
    # Adjustment objects to control the scrolling
    self._hadjustment = hadjustment
    self._vadjustment = vadjustment

    # Drawer object: the one that actually draws things in the buffer
    self.drawer = drawer

    # Margin of the buffer outside the screen view (a big value increases
    # the amount of memory required, but reduces the need of redrawing the
    # picture when scrolling or zooming in).
    self.buf_margin = margin = Point()
    margin.set_xy(buf_margin)
    margin.trunc(0.0, 1.0, 0.0, 1.0)

    self.buf = None              # The buffer where the output is first drawn
    self.buf_view = None         # The view for the buffer
    self.buf_needs_update = True # Whether the buffer needs to updating
    self.screen_view = None      # The view for the DrawableArea
    self.last_view = None        # The view as returned by the drawing routine
    self.pic_bbox = None         # The bounding box of the picture

    # Magnification in pixel per unit coordinate (positive).
    # Controls how much the picture is zoomed in the current screen view.
    # None means "show the picture in full in the current view".
    self.magnification = None
    self.last_magnification = None  # Magnification of most recent view 
    self.zoom_factor = 1.5          # Zoom factor

    self.scrollbar_page_inc = Point(0.9, 0.9) # Page increment for scrollbars
    self.scrollbar_step_inc = Point(0.1, 0.1) # Step increment for scrollbars

    # Signal handlers for the value-changed signal of the two adjustment
    # objects used for the horizontal and vertical scrollbars
    self._hadj_valchanged_handler = None
    self._vadj_valchanged_handler = None

    gtk.DrawingArea.__init__(self)

    gobject.signal_new("set-scroll-adjustment", self.__class__,
                       gobject.SIGNAL_RUN_LAST,
                       gobject.TYPE_NONE,
                       (gtk.Adjustment, gtk.Adjustment))

    self.set_set_scroll_adjustments_signal("set-scroll-adjustment")

    self.connect("expose_event", self.expose)
    self.connect("set-scroll-adjustment", ZoomableArea.scroll_adjustment)
    self.connect("configure_event", self.size_change)

  def get_hadjustment(self):
    return self._hadjustment

  def get_vadjustment(self):
    return self._vadjustment

  def set_hadjustment(self, adjustment):
    self._hadjustment = adjustment

  def set_vadjustment(self, adjustment):
    self._vadjustment = adjustment

  hadjustment = property(get_hadjustment, set_hadjustment)
  vadjustment = property(get_vadjustment, set_vadjustment)

  def zoom_here(self, scale_factor=None):
    """Fixes the magnification of the picture, exiting the picture view mode,
    i.e. the mode in which the picture fits exactly in the current screen
    view. The function apparently doesn't do anything, but it actually does,
    After the function has been called, the picture is no more resized to fit
    in the current view!"""
    lm = self.magnification
    if lm == None:
      lm = self.last_magnification
      if lm == None:
        return

    elif scale_factor == None:
      return

    self.magnification = (lm*scale_factor if scale_factor != None else lm)

  def zoom_in(self):
    """Zoom in."""
    self.zoom_here(self.zoom_factor)
    self.buf_needs_update = True
    self.queue_draw()

  def zoom_out(self):
    """Zoom out."""
    self.zoom_here(1.0/self.zoom_factor)
    self.buf_needs_update = True
    self.queue_draw()

  def zoom_off(self):
    """Return in full picture view, where the picture is scaled to fit
    the available portion of the screen (the DrawableArea)."""
    self.magnification = None
    self.buf_needs_update = True
    self.queue_draw()

  def size_change(self, myself, event):
    """Called when the DrawableArea changes in size. When that happens we
    exit full picture mode, by calling self.zoom_here(). The other option
    would be to keep rebuilding the picture from scratch and resize it so
    that it fits in the current area. That, however, would be expensive in
    terms of computation (and it is not always what the user may want to do).
    We rather provide a method to restore full picture mode and readjust the
    view (self.zoom_off)."""
    self.zoom_here()

  def get_buf_size(self):
    """Return the current size of the buffer (or (0, 0) if the buffer has
    not yet been allocated."""
    return get_pixbuf_size(self.buf)
  
  def alloc_buffer(self, copy_old=False, only_if_smaller=True):
    """(internal) Updates the buffer to the new window size.
    Called whenever the screen view size changes (for example, when the user
    maximises the window). The old buffer is reused if possible, unless
    only_if_smaller=True is given. If copy_old=True the part of the old buffer
    overlapping with the new one is copied back to the new buffer."""

    lx, ly = self.window.get_size()
    bsx, bsy = self.get_buf_size()

    # If the old buffer contains the new visible area we just exit
    if only_if_smaller and bsx >= lx and bsy >= ly:
        return

    # Otherwise we allocate a new buffer
    fx, fy = (int(self.buf_margin.x*lx),
              int(self.buf_margin.y*ly))
    new_bsx, new_bsy = (2*fx + lx, 2*fy + ly)
    debug_msg("DrawableArea has size %s x %s" % (lx, ly))
    debug_msg("Creating buffer with size %s x %s" % (new_bsx, new_bsy))
    new_buf = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, False, 8,
                             new_bsx, new_bsy)

    # And, if required, we copy from the old buffer the overlapping content
    if copy_old:
      # The old buffer goes at the center of the new one
      old_buf = self.buf
      old_bsx, old_bsy = (old_buf.get_width(), old_buf.get_height())
      old_sub_xmin, new_sub_xmin, inters_xsize = \
        central_segments_intersection(old_bsx, new_bsx)
      old_sub_ymin, new_sub_ymin, inters_ysize = \
        central_segments_intersection(old_bsy, new_bsy)

      old_buf.copy_area(old_sub_xmin, old_sub_ymin,
                        inters_xsize, inters_ysize,
                        new_buf, new_sub_xmin, new_sub_ymin)

    self.buf = new_buf
    self.buf_needs_update = True

  def get_visible_buf(self):
    """(internal) Get the visible part of the buffer (a subpixbuf)."""
    assert self.buf != None, "Cannot use get_visible_buf: buffer not present!"
    lx, ly = self.window.get_size()
    bsx, bsy = self.get_buf_size()
    marginx, marginy = ((bsx - lx)/2, (bsy - ly)/2)
    return self.buf.subpixbuf(marginx, marginy, lx, ly)

  def redraw_buffer(self):
    """Redraw the picture and update the internal buffer."""

    # First, we check whether we have a large-enough buffer
    self.alloc_buffer()
    self.buf.fill(color_background) # we clean it

    # We now have two cases depending whether or not we have the view
    if self.magnification == None:
      # We don't have the view:
      #   we want to draw all the picture in the DrawableArea

      # We then create a subpixbuf of the visible area of the buffer
      visible_of_buf = self.get_visible_buf()
      pix_size = get_pixbuf_size(visible_of_buf)
      pic_bbox, view = self.drawer.update(visible_of_buf, pix_size)
      self.last_view = view
      self.last_magnification = abs(pix_size.x/view.get_dx())
      self.pic_bbox = pic_bbox

    else:
      buf = self.buf
      pix_size = get_pixbuf_size(buf)
      coord_view = \
        self.last_view.new_reshaped(pix_size, 1.0/self.magnification)
      pic_bbox, view = \
        self.drawer.update(buf, pix_size, coord_view=coord_view)
      self.last_view = view
      self.last_magnification = abs(pix_size.x/view.get_dx())
      self.pic_bbox = pic_bbox

  def update_buffer(self):
    # Check whether we have a large-enough buffer
    self.alloc_buffer()

    if not self.buf_needs_update:
      # Determine whether we need to update the buffer
      return

    self.redraw_buffer()
    self.buf_needs_update = False

  def expose(self, draw_area, event):
    """Expose callback for the drawing area."""

    self.update_buffer()

    ea = event.area
    visible_of_buf = self.get_visible_buf()
    buf_area = visible_of_buf.subpixbuf(ea.x, ea.y, ea.width, ea.height)
    rowstride = buf_area.get_rowstride()
    pixels = buf_area.get_pixels()
    draw_area.window.draw_rgb_image(draw_area.style.black_gc,
                                    ea.x, ea.y, ea.width, ea.height,
                                    'normal', pixels, rowstride,
                                    ea.x, ea.y)

    self._update_scrollbars()
    return True

  def get_visible_coords(self):
    """Return the coordinate view corresponding to the DrawableArea."""
    if self.magnification != None:
      pix_size = Point(self.window.get_size())
      return self.last_view.new_reshaped(pix_size, 1.0/self.magnification)

    else:
      return self.last_view.new_scaled(1.0)

  def _update_scrollbars(self):
    """(internal) Update the ranges and positions of the scrollbars."""
    ha = self._hadjustment
    va = self._vadjustment

    self._block_scrollbar_signals(True)

    if self.pic_bbox != None and self.magnification != None:
      visible_coords = self.get_visible_coords()
      print "_update_scrollbars:", visible_coords
      pic_bbox = self.pic_bbox
      bb1 = self.pic_bbox.corner1
      bb2 = self.pic_bbox.corner2

      if self._hadjustment:
        page_size = abs(visible_coords.get_dx())
        ha.set_lower(0.0)
        ha.set_upper(abs(pic_bbox.get_dx()))
        ha.set_value(abs(visible_coords.corner1.x - bb1.x))
        ha.set_page_size(page_size)
        ha.set_page_increment(page_size*self.scrollbar_page_inc.x)
        ha.set_step_increment(page_size*self.scrollbar_step_inc.x)

      if self._vadjustment:
        page_size = abs(visible_coords.get_dy())
        va.set_lower(0.0)
        va.set_upper(abs(pic_bbox.get_dy()))
        va.set_value(abs(visible_coords.corner2.y - bb2.y))
        va.set_page_size(page_size)
        va.set_page_increment(page_size*self.scrollbar_page_inc.y)
        va.set_step_increment(page_size*self.scrollbar_step_inc.y)

    else:
      ha.set_lower(0.0)
      ha.set_upper(1.0)
      ha.set_value(0.0)
      ha.set_page_size(1.0)

      va.set_lower(0.0)
      va.set_upper(1.0)
      va.set_value(0.0)
      va.set_page_size(1.0)

    self._block_scrollbar_signals(False)

  def scroll_adjustment(self, hadjustment, vadjustment):
    self._hadjustment = hadjustment
    self._vadjustment = vadjustment
    if isinstance(hadjustment, gtk.Adjustment):
      self._hadj_valchanged_handler = \
        hadjustment.connect("value-changed", self._adjustments_changed)
    if isinstance(vadjustment, gtk.Adjustment):
      self._vadj_valchanged_handler = \
        vadjustment.connect("value-changed", self._adjustments_changed)

  def _block_scrollbar_signals(self, value):
    """(internal) When the user resizes the window, the view changes and
    hence the scrollbars are updated: update view ==> update scrollbars.
    However, when the user changes the scrollbars we should update the
    view: update scrollbars ==> update view. This would then lead to an
    infinite circular loop of updating: changing the view updates the
    scrollbars, which requires updating the view, etc.
    This method allows to lock the value-changed methods of the adjustment
    objects to avoid this circular loop."""

    if value:
      self._hadjustment.handler_block(self._hadj_valchanged_handler)
      self._vadjustment.handler_block(self._vadj_valchanged_handler)

    else:
      self._hadjustment.handler_unblock(self._hadj_valchanged_handler)
      self._vadjustment.handler_unblock(self._vadj_valchanged_handler)

  def _adjustments_changed(self, adjustment):
    visible_coords = self.get_visible_coords()
    print "_adjustments_changed:", visible_coords
    pic_bbox = self.pic_bbox

    p1 = pic_bbox.corner1
    p2 = pic_bbox.corner2
    v1 = visible_coords.corner1
    v2 = visible_coords.corner2

    ha = self._hadjustment
    dx = p1.x + sgn(visible_coords.get_dx())*ha.get_value() - v1.x

    va = self._vadjustment
    dy = p2.y + sgn(visible_coords.get_dy())*va.get_value() - v2.y
    print dy, va.get_value(), va.get_lower(), va.get_upper()

    dr = Point(dx, dy)
    self.last_view.translate(dr)
    self.buf_needs_update = True
    self.queue_draw()
