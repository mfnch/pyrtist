# Copyright (C) 2010, 2020
#  by Matteo Franchin (fnch@users.sourceforge.net)
#
# This file is part of Pyrtist.
#
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 2.1 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

from gi.repository import Gtk

class RotPaned(object):
  def __init__(self, child1=None, child2=None, rotation=0):
    self.child1 = child1
    self.child2 = child2
    self.rotation = rotation
    self._container = None
    self._vpaned = None
    self._hpaned = None
    self.paned = None

    self.rotate(rotation)

  def get_vpaned(self):
    if self._vpaned is not None:
      return self._vpaned
    self._vpaned = vp = Gtk.VPaned()
    return vp

  def get_hpaned(self):
    if self._hpaned is not None:
      return self._hpaned
    self._hpaned = hp = Gtk.HPaned()
    return hp

  def get_container(self):
    if self._container is not None:
      return self._container
    self._container = hb = Gtk.HBox()
    return hb

  def rotate(self, rotation=None):
    if rotation is None:
      rotation = (self.rotation + 1) % 4
    else:
      rotation = rotation % 4
    prev_paned = self.paned
    use_vert_paned = (rotation % 2 == 0)
    cur_paned = self.get_vpaned() if use_vert_paned else self.get_hpaned()

    # Hide all object in main container before doing modifications
    container = self.get_container()
    container.hide()

    # Remove children in previous paned
    if prev_paned is not None:
      for child in prev_paned.get_children():
        prev_paned.remove(child)

    if rotation < 2:
      c1, c2 = (self.child1, self.child2)
    else:
      c1, c2 = (self.child2, self.child1)

    do_resize = do_shrink = True
    cur_paned.pack1(c1, do_resize, do_shrink)
    cur_paned.pack2(c2, do_resize, do_shrink)

    # Show the right paned on the main container
    for child in container.get_children():
      container.remove(child)
    do_expand = do_fill = True
    zero_padding = 0
    container.pack_start(cur_paned, do_expand, do_fill, zero_padding)

    container.show_all()

    self.rotation = rotation
    self.paned = cur_paned
