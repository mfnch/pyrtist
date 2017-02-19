# Copyright (C) 2008-2010, 2017  by Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Pyrtist.
#
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

"""
Implementation of the ZoomableArea object, which adds buffered rendering,
zooming and scrolling capabilities to a DrawableArea object.
"""

__all__ = ["ImageDrawer", "DrawSucceded", "DrawStillWorking", "DrawFailed",
           "View", "ZoomableArea"]


import os
import time

import pygtk
pygtk.require('2.0')
import gtk
import gtk.gdk
import gobject

from .config import debug, debug_msg
from .geom2 import *
from .callbacks import Callbacks

color_background = 0x808080ff


class DrawState(object):
  pass


class DrawFailed(DrawState):
  pass


class DrawSucceded(DrawState):
  def __init__(self, bounding_box, view):
    self.bounding_box = bounding_box
    self.view = view


class DrawStillWorking(DrawState):
  def __init__(self, imagedrawer, killer=None):
    self.killer = killer
    self.imagedrawer = imagedrawer

  def kill(self):
    """Kill the running drawing process if it is still running."""
    if self.killer is not None:
      self.killer()
      self.killer = None


class ImageDrawer(object):
  def __init__(self):
    self._finished_callback = None

  def set_on_finish_callback(self, callback):
    self._finished_callback = callback

  def finished_drawing(self, state):
    """Function that 'update' may use in case it returns DrawStillWorking.
    The method 'update' can indeed do the drawing in a separate thread and
    let the execution of the program continue while the drawing is made.
    To notify that the drawing has finished, the separate thread has to call
    the method finished_drawing(state), passing the state of the operation
    (either DrawSucceded or DrawFailed).
    """
    assert isinstance(state, (DrawSucceded, DrawFailed))
    if self._finished_callback is not None:
      self._finished_callback(self, state)

  def update(self, pixbuf_output, pix_view, coord_view=None):
    """This function is called by ZoomableArea to update the view buffer.
    The function is required to carry out one of the two following tasks:
     1. 'coord_view' is not provided. The function receives the size of the
        buffer in 'pix_view' and has to draw the picture in the best possible
        way in the available space (possibly, increasing the image size until
        the picture hits one of the boundaries);
     2. 'coord_view' is provided. The function has to draw just the provided
        coordinate view of the picture, producing an output filling the whole
        buffer.
    The function is required to return one of the following objects:
     1. DrawSucceded(bounding_box, view): if the drawing operation succeded
        then the function returns this object specifying:
        - bounding_box: the bounding box in a geom2.Rectangle object;
        - view: the view in a View object;
     2. DrawFailed(): if the drawing operation failed, then this has to be
        communicated to the parent ZoomableObject by returning this object;
     3. DrawStillWorking(imagedrawer, killer): if the drawing operation is
        taking too long then the 'update' method may want to create a separate
        thread and return, so that the GUI can respond to user's requests.
        In this case, the 'update' method is expected to return a
        DrawStillWorking object. 'imagedrawer' must be 'self' (i.e. the
        ImageDrawer object). 'killer' is a callback function that the
        ZoomableObject may want to call in order to abort the drawing
        operation (i.e. kill the thread which is performing the drawing
        operation). Note that it is very important that the drawing thread
        calls the method 'finished_drawing' to signal that the drawing
        operation has terminated. Please, read the documentation for that
        method.
    """
    raise NotImplementedError("ImageDrawer.update is not implemented")


def get_pixbuf_size(buf):
  """If buf is a PixBuf returns its size as a tuple (width, height).
  If buf is None return (0, 0)."""
  return (Point(buf.get_width(), buf.get_height())
          if buf is not None else Point(0, 0))


class View(Rectangle):
  """Class to map pixel coordinate to arbitrary coordinates."""

  def __init__(self, size_in_pixels=None, topleft_coord=None,
               botright_coord=None):
    super(View, self).__init__(topleft_coord, botright_coord)
    self.view_size = size_in_pixels
    if (size_in_pixels is not None and topleft_coord is not None
        and botright_coord is not None):
      self.reset()

  def copy(self):
    """Return a copy of the View object."""
    return View(Point(self.view_size),
                Point(self.corner1), Point(self.corner2))

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
    return (x1 + pix[0]*dx_dpx, y1 + pix[1]*dy_dpy)

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


class ZoomableArea(gtk.DrawingArea):
  set_scroll_adjustment_signal_id = None

  def __init__(self, drawer,
               hadjustment=None, vadjustment=None,
               zoom_factor=1.5, zoom_margin=0.25, buf_margin=0.25,
               config=None, callbacks=None):

    super(ZoomableArea, self).__init__()

    # Drawer object: the one that actually draws things in the buffer
    self.drawer = drawer
    self.drawer_state = None

    # Adjustment objects to control the scrolling
    self._hadjustment = hadjustment
    self._vadjustment = vadjustment

    self.zoom_factor = zoom_factor  # Zoom factor

    zoom_margin = Point(zoom_margin)
    zoom_margin.trunc(0.0, 1.0, 0.0, 1.0)
    self.extra_zoom = zoom_margin + zoom_margin + Point(1.0, 1.0)
                                    # ^^^ Extra zoom (to move a little bit
                                    #     beyond the bounding box)

    # Margin of the buffer outside the screen view (a big value increases
    # the amount of memory required, but reduces the need of redrawing the
    # picture when scrolling or zooming in).
    self.buf_margin = margin = Point(buf_margin)
    margin.trunc(0.0, 1.0, 0.0, 1.0)

    # Callbacks to notify about events concerning the ZoomableArea
    self.callbacks = Callbacks.share(callbacks)
    self.callbacks.default("set_script_killer")

    self.scrollbar_page_inc = Point(0.9, 0.9) # Page increment for scrollbars
    self.scrollbar_step_inc = Point(0.1, 0.1) # Step increment for scrollbars

    self.reset()

    # Signal handlers for the value-changed signal of the two adjustment
    # objects used for the horizontal and vertical scrollbars
    self._hadj_valchanged_handler = None
    self._vadj_valchanged_handler = None

    if not ZoomableArea.set_scroll_adjustment_signal_id:
      ZoomableArea.set_scroll_adjustment_signal_id = \
        gobject.signal_new("set-scroll-adjustment", self.__class__,
                           gobject.SIGNAL_RUN_LAST,
                           gobject.TYPE_NONE,
                           (gtk.Adjustment, gtk.Adjustment))

    self.set_set_scroll_adjustments_signal("set-scroll-adjustment")

    self.connect("expose_event", self.expose)
    self.connect("set-scroll-adjustment", ZoomableArea.scroll_adjustment)
    self.connect("configure_event", self.size_change)

  def reset(self):
    """Bring the object to its initial state.
    Useful when the picture which is been shown suddenly changes
    (the coordinates changes, etc.)
    """
    self.kill_drawer()           # Stop the drawing process, if required

    self.buf = None              # The buffer where the output is first drawn
    self.buf_view = None         # The view for the buffer
    self.buf_needs_update = True # Whether the buffer needs updating
    self.last_view = None        # The view as returned by the drawing routine
    self.pic_bbox = None         # The bounding box of the picture
    self.pic_view = None         # The view of the picture

    # Magnification in pixel per unit coordinate (positive).
    # Controls how much the picture is zoomed in the current screen view.
    # None means "show the picture in full in the current view".
    self.magnification = None
    self.last_magnification = None  # Magnification of most recent view

  def __del__(self):
    self.kill_drawer()

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

  def has_box_coords(self):
    """Returns True only when the window was already built previously
    and the Box coordinate system is defined. Note, that this may be True even
    if the last call to Box failed and the window is currently "broken".
    """
    return (self.last_view is not None)

  def zoom_here(self, scale_factor=None):
    """Exit the picture view mode, i.e. the mode in which the picture fits
    exactly in the current screen view. The effect of this function may not be
    evident at first, but after the function has been called, the picture is no
    more resized to fit in the current view. One consequence is that scrolling
    becomes possible.
    """
    lm = self.magnification
    if lm is None:
      lm = self.last_magnification
      if lm is None:
        return
    elif scale_factor is None:
      return

    self.magnification = (lm*scale_factor if scale_factor is not None else lm)
    self.adjust_view()

  def adjust_view(self):
    """Adjust the view in case it falls outside the bounding box."""
    if self.magnification is not None:
      dr = rectangle_adjustment(self.pic_view, self.get_visible_coords())
      self.last_view.translate(dr)

  def refresh(self):
    """Refresh the view."""
    self.buf_needs_update = True
    self.queue_draw()

  def kill_drawer(self):
    """Kill the drawer thread in case it is still running."""
    if isinstance(self.drawer_state, DrawStillWorking):
      self.drawer_state.kill()
      self.drawer_state = DrawFailed()

  def zoom_in(self):
    """Zoom in."""
    self.zoom_here(self.zoom_factor)
    self.refresh()

  def zoom_out(self):
    """Zoom out."""
    self.zoom_here(1.0/self.zoom_factor)
    self.refresh()

  def zoom_off(self):
    """Return in full picture view, where the picture is scaled to fit
    the available portion of the screen (the DrawableArea)."""
    self.magnification = None
    self.refresh()

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

  def alloc_buffer(self, copy_old=True, only_if_smaller=True):
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
    new_buf.fill(color_background) # we clean it

    # And, if required, we copy from the old buffer the overlapping content
    old_buf = self.buf
    if copy_old and old_buf is not None:
      # The old buffer goes at the center of the new one
      old_bsx, old_bsy = get_pixbuf_size(old_buf)
      old_sub_xmin, new_sub_xmin, inters_xsize = \
        central_segments_intersection(old_bsx, new_bsx)
      old_sub_ymin, new_sub_ymin, inters_ysize = \
        central_segments_intersection(old_bsy, new_bsy)

      old_buf.copy_area(old_sub_xmin, old_sub_ymin,
                        inters_xsize, inters_ysize,
                        new_buf, new_sub_xmin, new_sub_ymin)

    self.buf = new_buf

  def get_central_buf(self):
    """(internal) Get the central part of the buffer (a subpixbuf).
    The central part of the buffer is the one which is initially visible when
    the buffer is refreshed. The user can then move the visible area off the
    center. If the visible area stays inside the buffer, then no refresh
    occurs (the image is refreshed from the buffer). If the visible area gets
    outside the buffered one, then a refresh occurs and the new visible area
    is positioned at the center of the refreshed buffer again.
    """
    assert self.buf is not None, "Cannot use get_central_buf: buffer not present!"
    lx, ly = self.window.get_size()
    bsx, bsy = self.get_buf_size()
    marginx, marginy = ((bsx - lx)/2, (bsy - ly)/2)
    return self.buf.subpixbuf(marginx, marginy, lx, ly)

  def get_visible_buf(self):
    buf_view = self.buf_view
    if self.magnification is None:
      return self.get_central_buf()

    else:
      vis_view = self.get_visible_coords()
      c1x, c1y = buf_view.coord_to_pix(vis_view.corner1)
      c2x, c2y = buf_view.coord_to_pix(vis_view.corner2)
      px, py = (int(min(c1x, c2x)), int(min(c1y, c2y)))
      lx, ly = self.window.get_size()
      buf = self.buf
      if px + lx < buf.get_width() and py + ly < buf.get_height():
        return buf.subpixbuf(px, py, lx, ly)
      else:
        return None

  def _imagedrawer_update_finish(self, state, pix_view):
      self.pic_bbox = pic_bbox = state.bounding_box
      self.buf_view = view = state.view
      self.last_magnification = abs(pix_view.x/view.get_dx())
      self.buf_view = view
      self.pic_bbox = pic_bbox
      self.pic_view = pic_bbox.new_scaled(self.extra_zoom)
      # ^^^ we artificially expand the bounding box so that we can move the view
      #     outside the bounding box
      self.last_view = self.buf_view.copy()

  def _imagedrawer_update(self, pixbuf_output, pix_view, coord_view=None):
    if isinstance(self.drawer_state, DrawStillWorking):
      return

    refresh_on_finish = [False]

    def on_finish(imgdrawer, state):
      self.callbacks.call("set_script_killer", None)
      if isinstance(state, DrawSucceded):
        self._imagedrawer_update_finish(state, pix_view)
      self.drawer_state = state
      if refresh_on_finish[0]:
        self.queue_draw()

    self.drawer.set_on_finish_callback(on_finish)

    self.buf_needs_update = False
    state = self.drawer.update(pixbuf_output, pix_view, coord_view=coord_view)

    self.drawer_state = state
    if not isinstance(state, DrawState):
      raise ValueError("ImageDrawer.update function should return either "
                       "a DrawSucceded, a DrawStillWorking or a DrawFailed "
                       "object, but I got {}.".format(state))

    if isinstance(state, DrawStillWorking):
      refresh_on_finish[0] = True
      self.callbacks.call("set_script_killer", self.kill_drawer)

  def redraw_buffer(self):
    """Redraw the picture and update the internal buffer."""

    debug_msg("REDRAWING", " ")
    # First, we check whether we have a large-enough buffer
    self.alloc_buffer()

    # We now have two cases depending whether or not we have the view
    if self.magnification is None:
      # We don't have the view:
      #   we want to draw all the picture in the DrawableArea

      # We then create a subpixbuf of the visible area of the buffer
      visible_of_buf = self.get_central_buf()
      pix_size = get_pixbuf_size(visible_of_buf)
      self._imagedrawer_update(visible_of_buf, pix_size)

    else:
      buf = self.buf
      pix_size = get_pixbuf_size(buf)
      coord_view = \
        self.last_view.new_reshaped(pix_size, 1.0/self.magnification)
      self._imagedrawer_update(buf, pix_size, coord_view=coord_view)

  def update_buffer(self):
    # Check whether the buffer is large enough
    self.alloc_buffer()

    if not self.buf_needs_update:
      # Determine whether we need to update the buffer
      if self.magnification is None:
        return

      visible_area = self.get_visible_coords()
      if self.buf_view is not None and self.buf_view.contains(visible_area):
        return

    self.redraw_buffer()

  def repaint(self, x, y, width, height):
    """Just repaint the given area of the widget."""

    visible_of_buf = self.get_visible_buf()

    # If - for some reason - the sub-buffer is not available, we just give up
    # repainting.
    if visible_of_buf is None:
      return

    if not isinstance(self.drawer_state, DrawSucceded):
      # In case we could not redraw the image we comunicate it by making
      # the picture darker than what it should be
      new_buf = visible_of_buf.copy()
      dx = visible_of_buf.get_width()
      dy = visible_of_buf.get_height()
      new_buf = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, False, 8, dx, dy)
      visible_of_buf.copy_area(0, 0, dx, dy, new_buf, 0, 0)
      new_buf.saturate_and_pixelate(new_buf, 0.5, True)
      visible_of_buf = new_buf

    buf_area = visible_of_buf.subpixbuf(x, y, width, height)
    rowstride = buf_area.get_rowstride()
    pixels = buf_area.get_pixels()

    # The following hangs sometimes on a poll() syscall.
    # gdk_draw_rgb_image_dithalign seems to call gdk_flush, which hangs on
    # xcb_wait_for_reply().
    self.window.draw_rgb_image(self.style.black_gc,
                               x, y, width, height,
                               'normal', pixels, rowstride,
                               x, y)

  def expose(self, draw_area, event):
    """Expose callback for the drawing area."""
    assert draw_area == self
    self.update_buffer()
    ea = event.area
    self.repaint(ea.x, ea.y, ea.width, ea.height)
    self._update_scrollbars()
    return True

  def get_visible_coords(self):
    """Return the coordinate view corresponding to the DrawableArea."""
    last_view = self.last_view
    if last_view is None:
      return None
    if self.magnification is None:
      return last_view.new_scaled(1.0)
    pix_size = Point(self.window.get_size())
    return last_view.new_reshaped(pix_size, 1.0/self.magnification)

  def _update_scrollbars(self):
    """(internal) Update the ranges and positions of the scrollbars."""
    ha = self._hadjustment
    va = self._vadjustment

    self._block_scrollbar_signals(True)

    if self.pic_view is not None and self.magnification is not None:
      visible_coords = self.get_visible_coords()
      pic_view = self.pic_view
      bb1 = self.pic_view.corner1
      bb2 = self.pic_view.corner2

      if self._hadjustment:
        page_size = abs(visible_coords.get_dx())
        ha.lower = 0.0
        ha.upper = abs(pic_view.get_dx())
        ha.value = abs(visible_coords.corner1.x - bb1.x)
        ha.page_size = page_size
        ha.page_increment = page_size*self.scrollbar_page_inc.x
        ha.step_increment = page_size*self.scrollbar_step_inc.x

      if self._vadjustment:
        page_size = abs(visible_coords.get_dy())
        va.lower = 0.0
        va.upper = abs(pic_view.get_dy())
        va.value = abs(visible_coords.corner1.y - bb1.y)
        va.page_size = page_size
        va.page_increment = page_size*self.scrollbar_page_inc.y
        va.step_increment = page_size*self.scrollbar_step_inc.y

    else:
      ha.lower = 0.0
      ha.upper = 1.0
      ha.value = 0.0
      ha.page_size = 1.0

      va.lower = 0.0
      va.upper = 1.0
      va.value = 0.0
      va.page_size = 1.0

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
    pic_view = self.pic_view

    p1 = pic_view.corner1
    p2 = pic_view.corner2
    v1 = visible_coords.corner1
    v2 = visible_coords.corner2

    ha = self._hadjustment
    dx = p1.x + sgn(visible_coords.get_dx())*ha.get_value() - v1.x

    va = self._vadjustment
    dy = p1.y + sgn(visible_coords.get_dy())*va.get_value() - v1.y

    dr = Point(dx, dy)
    self.last_view.translate(dr)
    self.adjust_view()
    self.queue_draw()
