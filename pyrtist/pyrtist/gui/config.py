# Copyright (C) 2008-2017 by Matteo Franchin (fnch@users.sf.net)
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

import imp
import sys
import os
import tempfile
import atexit

import gtk

def debug():
  import sys
  from IPython.Shell import IPShellEmbed
  calling_frame = sys._getframe(1)
  IPShellEmbed([])(local_ns  = calling_frame.f_locals,
                   global_ns = calling_frame.f_globals)

def debug_msg(msg, nl="\n"):
  pass #sys.stdout.write(msg + nl)

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

# By default this is the text put inside a new program
# created with File->New
source_of_new_script = '''#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri

#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(0, 50)
bbox2 = Point(100, 0)
#!PYRTIST:REFPOINTS:END
from pyrtist.lib2d import *

w = Window()          # Create a new Window and set the
w.BBox(bbox1, bbox2)  # bounding box from p1 and p2.
#!PYRTIST:CURSOR:HERE
#w.save('out.png')    # Uncomment to save the figure.
gui(w)                # Show the result.
'''

def installation_path():
  # Borrowed from wxglade.py
  try:
    root = sys.argv[0] if platform_is_win_py2exe else __file__
    if os.path.islink(root):
      root = os.path.realpath(root)
    return os.path.dirname(os.path.abspath(os.path.join(root, '..')))
  except:
    sys.stdout.write("Problem determining data installation path.\n")
    sys.exit()

def icon_path(filename=None, big_buttons=None):
  base = os.path.join(installation_path(), 'icons')
  if big_buttons != None:
    size_subpath = ('32x32' if big_buttons == True else '24x24')
    base = os.path.join(base, size_subpath)
  return base if filename == None else os.path.join(base, filename)

def get_hl_path(filename=None):
  base = os.path.join(installation_path(), 'hl')
  return base if filename == None else os.path.join(base, filename)

def get_gui_lib_path(filename=None):
  base = installation_path()
  return base if filename == None else os.path.join(base, filename)

def get_example_files():
  """Return a list of example files for Pyrtist."""
  import glob
  pattern = os.path.join(installation_path(), "examples", "*.py")
  return glob.glob(pattern)

#-----------------------------------------------------------------------------
# Temporary files

_tmp_files = [[]] # Current created temporary files

def tmp_new_filename(base, ext="", filenames=None, remember=True):
  try:
    prefix = "pyrtist%d%s" % (os.getpid(), base)
  except:
    prefix = "pyrtist%s" % base
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


class ConfigOption(object):
  def __init__(self, name, default_val, desc=None):
    self.name = name
    self.default_value = default_val
    self.desc = desc

  def get_range(self):
    return self.name

  def get(self, val):
    return val

  def set(self, val):
    return val

  def normalize(self, val):
    return self.get(self.set(val))

  def __str__(self):
    return self.desc or ""


class StringOption(ConfigOption):
  def __init__(self, default_val, desc=None):
    ConfigOption.__init__(self, "String", default_val, desc=desc)


class FileOption(StringOption):
  def __init__(self, default_val, must_exist=False, desc=None):
    ConfigOption.__init__(self, "File", default_val, desc=desc)
    self.must_exist = must_exist


class DirOption(StringOption):
  def __init__(self, default_val=None, must_exist=False, desc=None):
    ConfigOption.__init__(self, "Dir", default_val, desc=desc)
    self.must_exist = must_exist


class ScalarOption(ConfigOption):
  def __init__(self, name, default_val,
               minimum=None, maximum=None, scalar_type=int, desc=None):
    ConfigOption.__init__(self, name, default_val, desc=desc)
    self.minimum = minimum
    self.maximum = maximum
    self.scalar_type = scalar_type

  def set(self, value):
    v = self.scalar_type(value)
    mn = self.minimum
    mx = self.maximum
    if mx != None and v > mx:
      v = mx
    elif mn != None and v < mn:
      v = mn
    return v

  def __str__(self):
    return ConfigOption.__str__(self)

  def get_range(self):
    mn = self.minimum
    mx = self.maximum
    idx = 2*int(mx != None) + int(mn != 0)
    return self.name + \
           ["",
            " greater than %s" % mn,
            " lower than %s" % mx,
            " between %s and %s" % (mn, mx)][idx]


class IntOption(ScalarOption):
  def __init__(self, default_val, minimum=None, maximum=None, desc=None):
    ScalarOption.__init__(self, "Int", default_val, minimum, maximum,
                          int, desc=desc)


class FloatOption(ScalarOption):
  def __init__(self, default_val, minimum=None, maximum=None, desc=None):
    ScalarOption.__init__(self, "Real", default_val, minimum, maximum,
                          float, desc=desc)


class BoolOption(ScalarOption):
  def __init__(self, default_val, desc=None):
    ConfigOption.__init__(self, "Bool", default_val, desc=desc)

  def set(self, value):
    v = str(value).lower()
    return (v in ["true", "yes", "1", "ok"])

  def get(self, value):
    return self.set(value)

  def get_range(self):
    return "boolean (either True or False)"


class EnumOption(ConfigOption):
  def __init__(self, key_val_pairs, desc=None):
    self.key_to_val = dict(key_val_pairs)
    self.val_to_key = dict((i, s) for s, i in key_val_pairs)
    ConfigOption.__init__(self, "Enum", key_val_pairs[0][1], desc=desc)

  def get(self, val):
    return self.val_to_key.get(val, self.val_to_key[self.default_value])

  def set(self, key):
    return self.key_to_val.get(key, self.default_value)

  def get_range(self):
    return "one of: " + ", ".join(self.key_to_val.keys())


class BoxerConfigParser(cfgp.SafeConfigParser):
  def __init__(self, defaults=None, default_config=[],
               user_cfg_file=None):
    cfgp.SafeConfigParser.__init__(self, defaults)

    # Generate default configuration
    default_config_dict = {}
    descs = {}
    for section, option, desc in default_config:
      section_dict = default_config_dict.setdefault(section, {})
      section_dict[option] = desc.default_value
      desc_section_dict = descs.setdefault(section, {})
      desc_section_dict[option] = desc

    self._default_config = default_config_dict
    self._option_descs = descs
    self.is_modified = False
    self.broken = False
    self.user_cfg_file = user_cfg_file

  def mark_as_broken(self, broken=True):
    """Marks the configuration as broken. Broken configurations are not saved
    by the methond 'save_configuration', unless the optional argument of the
    same method 'force' is set to True."""
    self.broken = broken

  def has_default_option(self, section, option):
    if section in self._default_config:
      return (option in self._default_config[section])
    else:
      return False

  def get_sections(self):
    """Get all the available sections."""
    return self._default_config.keys()

  def get_options(self, section):
    """Get all the available options for the given section."""
    return self._default_config[section].keys()

  def get_desc(self, section, option):
    """Get a description for the option in the section."""
    return self._option_descs[section][option]

  def get(self, section, option, raw=False, vars=None):
    if (not self.has_option(section, option)
        and self.has_default_option(section, option)):
      return self._default_config[section][option]

    else:
      return cfgp.SafeConfigParser.get(self, section, option, raw, vars)

  def remove_option(self, section, option):
    if self.has_option(section, option):
      self.is_modified = True
      cfgp.SafeConfigParser.remove_option(self, section, option)

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


_configuration = None

def get_configuration():
  global _configuration
  if _configuration:
    return _configuration

  # Create Boxer configuration directory
  home_path = os.path.expanduser("~")
  if platform_is_win:
    cfg_dir = "pyrtist_settings"
  else:
    cfg_dir = ".pyrtist"
  cfg_file = "config.cfg"

  # Create default box executable path
  box_exec = (os.path.join(installation_path(), "box", "bin", "box.exe")
              if platform_is_win else "box")

  default_config = \
   [('Box', 'exec',
     FileOption(box_exec, must_exist=True,
                desc='The path to the Box executable')),
    ('Box', 'stdout_buffer_size',
     IntOption(32, -1, 1024, desc=('Maximum size in KB of Box scripts stdout '
                                  '(negative for unlimited)'))),
    ('Box', 'stdout_update_delay',
     FloatOption(0.2, 0.1, 10.0,
                 desc=('Delay between updates of the textview showing the '
                       'output of Box programs'))),
    ('GUI', 'font',
     StringOption('Monospace 10', desc='The font used in the GUI')),
    ('GUI', 'rotation',
     EnumOption((("bottom", "0"), ("left", "1"), ("top", "2"), ("right", "3")),
                desc='The displacement of text-view with respect to the '
                     'drawing-area')),
    ('GUI', 'window_size',
     StringOption('600x600', desc='The initial size of the window')),
    ('GUI', 'big_buttons',
     BoolOption("False", desc='Whether the button size should be increased')),
    ('GUIView', 'refpoint_size',
     IntOption(4, 1, 50, desc='The size of the squares used to mark '
                              'the reference points')),
    ('Library', 'dir',
     DirOption(desc='Path to the local copy of the Pyrtist library')),
    ('Behaviour', 'button_left',
     IntOption(1, 1, 5, desc='ID of the mouse left button')),
    ('Behaviour', 'button_center',
     IntOption(2, 1, 5, desc='ID of the mouse central button')),
    ('Behaviour', 'button_right',
     IntOption(3, 1, 5, desc='ID of the mouse right button'))]

  user_cfg_file = os.path.join(home_path, cfg_dir, cfg_file)

  c = BoxerConfigParser(default_config=default_config,
                        user_cfg_file=user_cfg_file)
  successful_reads = c.read([user_cfg_file])

  _configuration = c
  return c


class ConfigurableData(dict):
  def __init__(self, parent=None, children=None):
    dict.__init__(self)
    self.parent = parent
    self.children = children


class Configurable(object):
  def __init__(self, config=None, from_args=None):
    self._config = data = ConfigurableData()

    if config != None:
      data.update(config)

    if from_args != None:
      data.update(from_args.get("config", {}))

  def __del__(self):
    pass

  def set_config_parent(self, parent):
    self._config.parent = parent

  def set_config_children(self, *children):
    self._config.children = children

  def set_config(self, *nameval_list, **nameval_dict):
    """Set the configuration option with name 'name' to 'value'."""
    n = len(nameval_list)
    assert n % 2 == 0, \
      "no value given for name %s" % nameval_list[-1]
    for i in range(0, n/2, 2):
      self._config[nameval_list[i]] = nameval_list[i + 1]
    self._config.update(nameval_dict)

  def set_config_default(self, *nameval_list, **nameval_dict):
    """Set the configuration option only if it is missing."""
    n = len(nameval_list)
    assert n % 2 == 0, \
      "no value given for name %s" % nameval_list[-1]
    for i in range(0, n/2, 2):
      self._config.setdefault(nameval_list[i], nameval_list[i + 1])
    for name, val in nameval_dict.iteritems():
      self._config.setdefault(name, val)

  def get_config(self, name, opt=None):
    """Return the value of the configuration option 'name' or 'value'
    if the configuration does not have such an option.
    """
    return self._config[name] if name in self._config else opt
