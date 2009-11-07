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
import ConfigParser as cfgp

box_syntax_highlighting = sys.path[0]

# By default this is the text put inside a new program
# created with File->New
box_source_of_new = \
"""w = Window[][
  .Show[(0, 0), (100, 50)]

  //!BOXER:CURSOR:HERE
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

class BoxerConfigParser(cfgp.SafeConfigParser):
  def __init__(self, defaults=None, default_config=[]):
    cfgp.SafeConfigParser.__init__(self, defaults)

    # Generate default configuration
    default_config_dict = {}
    for section, option, value in default_config:
      if default_config_dict.has_key(section):
        section_dict = default_config_dict[section]
      else:
        section_dict = {}
        default_config_dict[section] = section_dict
      section_dict[option] = value

    self._default_config = default_config_dict

  def has_default_option(self, section, option):
    if self._default_config.has_key(section):
      return self._default_config[section].has_key(option)
    else:
      return False

  def get(self, section, option, raw=False, vars=None):
    if (not self.has_option(section, option)
        and self.has_default_option(section, option)):
      return self._default_config[section][option]

    else:
      return cfgp.SafeConfigParser.get(self, section, option, raw, vars)

def get_configuration():
  # Create Boxer configuration directory
  home_path = os.path.expanduser("~")
  if platform_is_win:
    cfg_dir = "boxer_settings"
  else:
    cfg_dir = ".boxer"
  cfg_file = "config.cfg"

  # Create default box executable path
  if platform_is_win:
    box_exec = os.path.join(installation_path(), "box", "bin", "box.exe")
  else:
    box_exec = "box"

  default_config = \
   [('Box', 'exec', box_exec),
    ('GUI', 'font', 'Monospace 10'),
    ('GUIView', 'refpoint_size', '4'),
    ('Behaviour', 'button_left', '1'),
    ('Behaviour', 'button_center', '2'),
    ('Behaviour', 'button_right', '3'),
    ('Internals', 'src_marker_refpoints_begin', '//!BOXER:REFPOINTS:BEGIN'),
    ('Internals', 'src_marker_refpoints_end', '//!BOXER:REFPOINTS:END'),
    ('Internals', 'src_marker_cursor_here', '//!BOXER:CURSOR:HERE')]

  user_cfg_file = os.path.join(home_path, cfg_dir, cfg_file)

  c = BoxerConfigParser(default_config=default_config)
  successful_reads = c.read([user_cfg_file])

  return c

