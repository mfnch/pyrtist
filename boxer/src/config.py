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

import imp, sys, os, tempfile, atexit

def debug():
  import sys
  from IPython.Shell import IPShellEmbed
  calling_frame = sys._getframe(1)
  IPShellEmbed([])(local_ns  = calling_frame.f_locals,
                   global_ns = calling_frame.f_globals)

def debug_msg(msg):
  print msg

# Trick to determine whether we are running under Py2exe or not
def main_is_frozen():
  """Return True when running as a Py2exe executable."""
  return (hasattr(sys, "frozen") or # new py2exe
          hasattr(sys, "importers") # old py2exe
          or imp.is_frozen("__main__")) # tools/freeze

def get_main_dir():
  """Return the path to the main Python source from which the execution
  started."""
  if main_is_frozen():
    return os.path.dirname(sys.executable)
  return os.path.dirname(sys.argv[0])

# Platform information...
import platform as p
platform = p.system()
platform_is_win = (platform == "Windows")
platform_is_win_py2exe = main_is_frozen()

import ConfigParser as cfgp

box_syntax_highlighting = sys.path[0]

# By default this is the text put inside a new program
# created with File->New
box_source_of_new = \
"""//!BOXER:VERSION:0:1:1
include "g"
GUI = Void
Window@GUI[]

//!BOXER:REFPOINTS:BEGIN
bbox1 = Point[.x=0.0, .y=0.0]
bbox2 = Point[.x=100.0, .y=50.0]
//!BOXER:REFPOINTS:END
w = Window[][
  .Show[bbox1, bbox2]

  //!BOXER:CURSOR:HERE
]

GUI[w]
"""

def installation_path():
  # Borrowed from wxglade.py
  try:
    root = sys.argv[0] if platform_is_win_py2exe else __file__
    if os.path.islink(root):
      root = os.path.realpath(root)
    return os.path.dirname(os.path.abspath(root))
  except:
    print "Problem determining data installation path."
    sys.exit()

# Whether we should try to use threads
use_threads = True

def glade_path(filename=None):
  base = os.path.join(installation_path(), 'glade')
  if filename == None:
    return base
  else:
    return os.path.join(base, filename)

def get_example_files():
  """Return a list of example files for Boxer."""
  import glob
  pattern = os.path.join(installation_path(), "examples", "*.box")
  return glob.glob(pattern)

#-----------------------------------------------------------------------------
# Temporary files

_tmp_files = [[]] # Current created temporary files

def tmp_new_filename(base, ext="", filenames=None, remember=True):
  try:
    prefix = "boxer%d%s" % (os.getpid(), base)
  except:
    prefix = "boxer%s" % base
  filename = tempfile.mktemp(prefix=prefix, suffix="." + ext)
  if remember: _tmp_files[0].append(filename)
  if filenames != None:
    filenames.append(filename)
  return filename

def tmp_remove_files(file_names):
  """Remove all the temporary files created up to now."""
  all_files = _tmp_files[0]
  for filename in file_names:
    try:
      if filename in all_files:
        all_files.remove(filename)
      os.unlink(filename)
    except:
      pass

def tmp_remove_all_files():
  """Remove all the temporary files created up to now."""
  tmp_remove_files(_tmp_files[0])

# We try to register a cleanup function to delete temporary files
# That shouldn't be strictly neccessary, but keeps clean the system
# during development and in case of unexpected crashes (bugs).
try:
  atexit.register(tmp_remove_all_files)
except:
  pass

#-----------------------------------------------------------------------------
# Saving and loading the program settings (at startup and exit)

class BoxerConfigParser(cfgp.SafeConfigParser):
  def __init__(self, defaults=None, default_config=[],
               user_cfg_file=None):
    cfgp.SafeConfigParser.__init__(self, defaults)

    # Generate default configuration
    default_config_dict = {}
    for section, option, value, _ in default_config:
      if default_config_dict.has_key(section):
        section_dict = default_config_dict[section]
      else:
        section_dict = {}
        default_config_dict[section] = section_dict
      section_dict[option] = value

    self._default_config = default_config_dict
    self.is_modified = False
    self.broken = False
    self.user_cfg_file = user_cfg_file

  def mark_as_broken(self, broken=True):
    """Marks the configuration as broken. Broken configurations are not saved
    by the methond 'save_configuration', unless the optional argument of the
    same method 'force' is set to True."""
    self.broken = broken

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

  def set(self, section, option, value, auto_add_section=True):
    self.is_modified = True
    if not self.has_section(section):
        self.add_section(section)
    return cfgp.SafeConfigParser.set(self, section, option, value)

  def save_configuration(self, force=False, create_dir=True):
    if not self.is_modified:
      return

    if self.broken and not force:
      return

    if self.user_cfg_file != None:
      try:
        cfg_dir, _ = os.path.split(self.user_cfg_file)
        if not os.path.exists(cfg_dir):
          if create_dir:
            os.mkdir(cfg_dir)

      except:
        pass

      f = open(self.user_cfg_file, "w")
      self.write(f)
      f.close()


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
   [('Box', 'exec', box_exec, 'The path to the Box executable'),
    ('Box', 'stdout_buffer_size', '32', 'Maximum size in KB of Box scripts '
                                        'stdout (negative for unlimited)'),
    ('Box', 'stdout_update_delay', '0.2', 'Delay between updates of the '
                               'textview showing the output of Box programs'),
    ('GUI', 'font', 'Monospace 10', 'The font used in the GUI'),
    ('GUIView', 'refpoint_size', '4', 'The size of the squares used to mark '
                                      'the reference points'),
    ('Behaviour', 'button_left', '1', 'ID of the mouse left button'),
    ('Behaviour', 'button_center', '2', 'ID of the mouse central button'),
    ('Behaviour', 'button_right', '3', 'ID of the mouse right button'),
    ('Internals', 'src_marker_refpoints_begin', '//!BOXER:REFPOINTS:BEGIN',
                                                                        None),
    ('Internals', 'src_marker_refpoints_end', '//!BOXER:REFPOINTS:END', None),
    ('Internals', 'src_marker_cursor_here', '//!BOXER:CURSOR:HERE', None)]

  user_cfg_file = os.path.join(home_path, cfg_dir, cfg_file)

  c = BoxerConfigParser(default_config=default_config,
                        user_cfg_file=user_cfg_file)
  successful_reads = c.read([user_cfg_file])
  return c


class Configurable(object):
  def __init__(self, config=None, from_args=None):
    if from_args != None:
      self._config = from_args.get("config", {})
    elif config != None:
      self._config = config
    else:
      self._config = {}

  def set_config(self, name, value):
    """Set the configuration option with name 'name' to 'value'."""
    if type(name) == str:
      self._config[name] = value
    else:
      for n, v in name:
        self._config[n] = v

  def set_config_default(self, name, value=None):
    """Set the configuration option only if it is missing."""
    if type(name) == str:
      if name not in self._config:
        self._config[name] = value
    else:
      for n, v in name:
        if n not in self._config:
          self._config[n] = v

  def get_config(self, name, opt=None):
    """Return the value of the configuration option 'name' or 'value'
    if the configuration does not have such an option.
    """
    return self._config[name] if name in self._config else opt
