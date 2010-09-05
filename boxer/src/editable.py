# Copyright (C) 2010 by Matteo Franchin (fnch@users.sourceforge.net)
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

"""
Here we extend ZoomableArea so that it can show Box pictures, together with
the Boxer reference points and allows the user to insert/move/... them.
"""

from zoomable import ZoomableArea

class BoxViewArea(ZoomableArea):
  def __init__(self, filename=None, out_fn=None, **extra_args):
    # Create the Document
    self.document = d = document.Document()
    if filename != None:
      d.load_from_file(filename)

    # Create the Box drawer
    self.drawer = drawer = BoxImageDrawer(d)
    drawer.out_fn = out_fn if out_fn != None else sys.stdout.write

    # Create the ZoomableArea
    ZoomableArea.__init__(self, drawer, **extra_args)

class BoxEditableArea(object):
  pass
