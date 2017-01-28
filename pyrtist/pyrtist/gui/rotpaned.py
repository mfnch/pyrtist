# Copyright (C) 2010
#  by Matteo Franchin (fnch@users.sourceforge.net)
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

import gtk

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
    if self._vpaned != None:
      return self._vpaned

    else:
      self._vpaned = vp = gtk.VPaned()
      return vp

  def get_hpaned(self):
    if self._hpaned != None:
      return self._hpaned

    else:
      self._hpaned = hp = gtk.HPaned()
      return hp

  def get_container(self):
    if self._container != None:
      return self._container
    else:
      self._container = hb = gtk.HBox()
      return hb

  def rotate(self, rotation=None):
    if rotation == None:
      rotation = (self.rotation + 1) % 4
    else:
      rotation = rotation % 4
    prev_paned = self.paned
    use_vert_paned = (rotation % 2 == 0)
    cur_paned = self.get_vpaned() if use_vert_paned else self.get_hpaned()

    # Hide all object in main container before doing modifications
    container = self.get_container()
    container.hide_all()

    # Remove children in previous paned
    if prev_paned != None:
      for child in prev_paned.get_children():
        prev_paned.remove(child)

    if rotation < 2:
      c1, c2 = (self.child1, self.child2)
    else:
      c1, c2 = (self.child2, self.child1)

    cur_paned.pack1(c1, resize=True, shrink=True)
    cur_paned.pack2(c2, resize=True, shrink=True)

    # Show the right paned on the main container
    for child in container.get_children():
      container.remove(child)
    container.pack_start(cur_paned)

    container.show_all()

    self.rotation = rotation
    self.paned = cur_paned
