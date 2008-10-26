# Copyright (C) 2008 by Matteo Franchin (fnch@users.sourceforge.net)
#
# This file is part of Box.
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

import sys

box_syntax_highlighting = sys.path[0]

# By default this is the text put inside a new program
# created with File->New
box_source_of_new = \
"""w = Window[][
  .Show[(0, 0), (100, 50)]
]

GUI[w]
"""

class Config:
  """Class to store global configuration settings."""

  def __init__(self):
    self.default = {"font": "Monospace 10",
                    "ref_point_size": 4,
                    "src_marker_refpoints_begin": "//!BOXER:REFPOINTS:BEGIN",
                    "src_marker_refpoints_end": "//!BOXER:REFPOINTS:END",
                    "button_left": 1,
                    "button_center": 2,
                    "button_right": 3}

  def get_default(self, name, default=None):
    """Get a default configuration setting."""
    if self.default.has_key(name):
      return self.default[name]
    else:
      return default