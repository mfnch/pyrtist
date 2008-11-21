# Copyright (C) 2008 by Matteo Franchin (fnch@users.sourceforge.net)
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

import sys, os, os.path

box_syntax_highlighting = sys.path[0]

# By default this is the text put inside a new program
# created with File->New
box_source_of_new = \
"""w = Window[][
  .Show[(0, 0), (100, 50)]
]

GUI[w]
"""

def installation_path_linux():
  # Borrowed from wxglade.py
  try:
    root = __file__
    if os.path.islink(root):
      root = os.path.realpath(root)
    return os.path.dirname(os.path.abspath(root))
  except:
    print "Problem determining data installation path."
    sys.exit()

def installation_path_windows():
  try:
    root = sys.argv[0]
    if os.path.islink(root):
      root = os.path.realpath(root)
    return os.path.dirname(os.path.abspath(root))
  except:
    print "Problem determining data installation path."
    sys.exit()

installation_path = installation_path_linux

import platform as p
platform = p.system()
platform_is_win = (platform == "Windows")
platform_is_win_py2exe = platform_is_win # This may change in the future
if platform_is_win_py2exe:
  # This is what we need to use when running the py2exe
  # executable generated for Windows
  installation_path = installation_path_windows

def glade_path(filename=None):
  base = os.path.join(installation_path(), 'glade')
  if filename == None:
    return base
  else:
    return os.path.join(base, filename)

def get_example_files():
  """Return a list of example files for Boxer."""
  import glob
  pattern = os.path.join(installation_path(), "examples", "*")
  return glob.glob(pattern)

class Config:
  """Class to store global configuration settings."""

  def __init__(self):
    self.default = {"font": "Monospace 10",
                    "ref_point_size": 4,
                    "src_marker_refpoints_begin": "//!BOXER:REFPOINTS:BEGIN",
                    "src_marker_refpoints_end": "//!BOXER:REFPOINTS:END",
                    "src_marker_cursor_here": "//!BOXER:CURSOR:HERE",
                    "button_left": 1,
                    "button_center": 2,
                    "button_right": 3}

  def get_default(self, name, default=None):
    """Get a default configuration setting."""
    if self.default.has_key(name):
      return self.default[name]
    else:
      return default

  def box_executable(self):
    """Returns the absolute path of the Box executable."""
    default = self.get_default("box_executable")
    if default != None: return default
    if platform_is_win:
      return os.path.join(installation_path(), "box", "bin", "box.exe")
    else:
      return "box"
