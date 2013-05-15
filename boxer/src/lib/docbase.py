# Copyright (C) 2010-2011 Matteo Franchin (fnch@users.sourceforge.net)
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

import sys
import os
import time

import config
from config import Configurable
from exec_command import exec_command
from refpoints import RefPoint, RefPoints

# This is the version of the document (which may be different from the version
# of Boxer which was used to write it. We should save to file all this info,
# but we currently do not (have to improve this!)
version = (0, 2, 0)

max_chars_per_line = 79
marker_cursor_here = "//!BOXER:CURSOR:HERE"

#from os import linesep as endline
endline = "\n"

default_preamble = 'include "g"\nGUI = Void\nWindow@GUI[]\n\n'

default_code = "\n"

def cmp_version(v1, v2):
  """Compare two version tuples."""
  return cmp(v1[0], v2[0]) or cmp(v1[1], v2[1]) or cmp(v1[2], v2[2])

def text_writer(pieces, sep=", ", line_sep=None, max_line_width=None):
  """Similarly to str.join, this function concatenates the strings contained
  in the list ``pieces`` separating them with the separator ``sep``.
  However, the width of the line is always kept below ``max_line_width``,
  if possible. This means that when a line would exceed this limit, a new line
  is started.
  NOTE: each element of pieces is filtered through the function str() before
    being used.
  """

  mlw = max_line_width
  if mlw == None:
    mlw = max_chars_per_line
  lsep = line_sep
  if lsep == None:
    lsep = endline
  text = ""

  this_line = ""
  cur_sep = ""
  for piece in pieces:
    p = str(cur_sep) + str(piece)
    lp = len(p)
    lts = len(this_line)
    if lts == 0 or lts + lp <= mlw:
      this_line += p
      cur_sep = sep

    else:
      text += this_line + lsep
      this_line = str(piece)
      cur_sep = sep

  if len(this_line) > 0:
    text += this_line + lsep
  return text


def parse_given_eq_smthg(s, fixed_part):
  """Parse the given string 's' assuming it has the form
  "fixed_part = something" where fixed_part must match the second argument
  received by the function. Return 'something' if this is the case, otherwise
  return None."""
  left, right = s.split("=", 1)
  if left.strip() == fixed_part:
    return right.strip()
  else:
    return None

def refpoint_from_string_latest(s):
  """Parse a string representation of a GUIPoint and return the corresponding
  GUIPoint object."""
  try:
    lhs, rhs = s.split("=", 1)
    rhs, s_next = rhs.split("]", 1)
    point_str, rem_str = rhs.split("[", 1)
    if point_str.strip() != "Point":
      return None
    str_x, str_y = rem_str.split(",", 1)
    x = float(parse_given_eq_smthg(str_x, ".x"))
    y = float(parse_given_eq_smthg(str_y, ".y"))
    return RefPoint(lhs.strip(), [x, y])

  except:
    return None

def refpoint_from_string_v0_1_0(s):
  """Similar to parse_guipoint_new, but for Boxer 0.1."""
  try:
    lhs, rhs = s.split("=", 1)
    rem_str, _ = rhs.split(")", 1)
    rem_str, x_y_str = rem_str.split("(", 1)
    if rem_str.strip() != "":
      return None
    str_x, str_y = x_y_str.split(",", 1)
    return RefPoint(lhs.strip(), [float(str_x), float(str_y)])

  except Exception as x:
    return None

def refpoint_from_string(s):
  """Try to parse a representation of a RefPoint first with parse_guipoint_new
  and then with parse_guipoint_v0_1_0 if the former failed."""
  rp = refpoint_from_string_latest(s)
  if rp != None:
    return rp
  else:
    return refpoint_from_string_v0_1_0(s)

def refpoint_to_string(rp, version=version):
  """Return a string representation of the RefPoint according to the given
  Boxer version. The returned string can be parsed by 'refpoint_from_string'.
  """
  if version == (0, 1):
    return "%s = (%s, %s)" % (rp.name, rp.value[0], rp.value[1])
  elif cmp_version(version, (0, 1, 1)) <= 0:
    return ("%s = Point[.x=%s, .y=%s]"
            % (rp.name, rp.value[0], rp.value[1]))    
  else:
    return rp.get_box_source()

def search_first(s, things, start=0):
  found = -1
  for thing in things:
    i = s.find(thing, start)
    if found == -1:
      found = i
    elif i != -1 and i < found:
      found = i
  return found


def match_close(src, start, pars):
  open_bracket = src[start]
  close_bracket = pars[open_bracket]
  while True:
    next = search_first(src, [open_bracket, close_bracket], start + 1)
    if next == -1:
      raise "Missing closing parethesis!"
    if src[next] == open_bracket:
      start = match_close(src, next) + 1
    else:
      return next + 1

def get_next_guipoint_string(src, start=0, seps=[",", endline],
                             pars={"(": ")", "[": "]"}):
  pos = start
  open_seps = pars.keys()
  all_seps = seps + open_seps
  while True:
    next = search_first(src, all_seps, pos)
    if next < 0:
      # no comma nor parenthesis: we are parsing the last bit
      return (src[start:], len(src))

    c = src[next]
    if c in seps:
      # comma comes before next open parenthesis
      return (src[start:next], next + 1)

    elif c in open_seps:
      pos = match_close(src, next, pars=pars)

def parse_guipoint_part(src, guipoint_fn, start=0):
  if len(src.strip()) > 0:
    while start < len(src):
      line, start = get_next_guipoint_string(src, start)
      line = line.strip()
      if len(line) > 0:
        guipoint_fn(line)

# Enumerate the different possible ways of building a representation
# for a Document:
# - MODE_ORIG: original, as given by the user;
# - MODE_STORE: as it should be when saving it to disk;
# - MODE_EXEC: as it should be when executed within the GUI.
(MODE_ORIG,
 MODE_STORE,
 MODE_EXEC) = range(3)


class DocumentBase(Configurable):
  """Class to load, save, display and edit Boxer documents.
  A Boxer document is basically a Box program plus some metainfo contained
  in comments."""

  accepted_versions = []         # Versions accepted by the Document class

  def __init__(self, filename=None, callbacks=None,
               config=None, from_args=None):
    """filename=None :the filename associated to the document."""
    Configurable.__init__(self, config=config, from_args=from_args)

    self.filename = None         # File name corresponding to this document
    self.version = None          # Version of the file format
    self.preamble = None         # Preamble of the document
    self.usercode = None         # Body of the document

    if callbacks == None:
      callbacks = {}
    callbacks.setdefault("box_document_execute", None)
    callbacks.setdefault("box_document_executed", None)
    callbacks.setdefault("box_exec_output", None)

    self._fns = callbacks
    self.refpoints = RefPoints(callbacks=self._fns)

  def notify(self, t, msg):
    '''Used to notify problems during load/save.'''
    sys.stdout.write("%s: %s\n" % (t, msg))

  def new(self, preamble=None, refpoints=[], code=None):
    if code == None:
      code = default_code
    if preamble == None:
        preamble = default_preamble
    self.refpoints.load(refpoints)
    self.preamble = preamble
    self.usercode = code
    self.version = version

  def get_refpoints(self):
    '''Get a list of the reference points in the document (list of GUIPoint)'''
    return self.refpoints

  def get_user_code(self):
    return self.usercode

  def get_preamble(self):
    return self.preamble

  def get_boot_code(self, preamble=None):
    return preamble if preamble != None else self.preamble

  def set_boot_code(self, boot_code):
    """Set the content of the additional Box code used by Boxer."""
    self.preamble = boot_code

  def set_user_code(self, code):
    """Set the content of the user Box code."""
    self.usercode = code

  def get_part_version(self):
    """Produce the version line."""
    raise NotImplementedError("get_part_version")

  def get_part_boot_code(self, boot_code):
    """Produce the boot code (auxiliary code used to communicate the output
    back to the GUI).
    """
    raise NotImplementedError("get_part_boot_code")

  def get_part_def_refpoints(self):
    """Produce the part of the file which stores information about the
    reference points.
    """
    raise NotImplementedError("get_part_boot_code")

  def get_part_preamble(self, mode=None, boot_code=None):
    """Return the first part of the Box file, where metainformation such as
    the version of the file format and the reference points are defined.
    """
    if mode == MODE_STORE:
      part1 = self.get_part_version()
      part2 = self.get_part_boot_code(boot_code)
      part3 = self.get_part_def_refpoints()
      return endline.join([part1, part2, part3])

    else:
      return self.get_part_boot_code(boot_code)

  def get_part_user_code(self, mode=None):
    """Get the user part of the Box source."""
    return self.usercode

  def new(self):
    '''Start working on a new document (un-named).'''
    self.filename = None

  def save_to_str(self, version=version):
    '''Build the file corresponding to the document and return it as a string.
    '''
    part1 = self.get_part_preamble(mode=MODE_STORE)
    part2 = self.get_part_user_code(mode=MODE_STORE)
    return part1 + part2

  def load_from_str(self, boxer_src):
    raise NotImplementedError("DocumentBase.load_from_str")

  def load_from_file(self, filename, remember_filename=True):
    f = open(filename, "r")
    self.load_from_str(f.read())
    f.close()

    if remember_filename:
      self.filename = filename

  def save_to_file(self, filename, version=version, remember_filename=True):
    f = open(filename, "w")
    f.write(self.save_to_str(version=version))
    f.close()

    if remember_filename:
      self.filename = filename

  def box_query(self, variable):
    """Obtain the value of 'variable' by calling the Box executable
    associated to this document with the option '-q'. This is useful to obtain
    information about the configuration of the compiler which is executing the
    document.
    NOTE: at the moment the choice of the Box compiler is global, but the plan
    is to make this choice document-wise (we may even store a comment into the
    document source to specify which Box version to use. This may then be used
    to discriminate between different Box executable to call)."""
    box_executable = self.get_config("box_executable", "box")
    box_args = ["-q", str(variable)]

    box_finished = []
    def do_at_exit():
      box_finished.append(True)

    box_output = []
    def out_fn(line):
      box_output.append(line)

    killer = exec_command(box_executable, box_args,
                          out_fn=out_fn, do_at_exit=do_at_exit)

    while not box_finished:
      time.sleep(0.1)

    return "".join(box_output).rstrip()

  def execute(self, preamble=None, out_fn=None, exit_fn=None):
    # Have to find a convenient way to pass:
    # - extra command line arguments
    # - temporary directory, etc
    fn = self._fns["box_document_execute"]
    if fn != None:
      fn(self, preamble, out_fn, exit_fn)

    tmp_fns = []
    src_filename = config.tmp_new_filename("source", "box", tmp_fns)
    presrc_filename = config.tmp_new_filename("pre", "box", tmp_fns)

    original_userspace = self.get_part_user_code(mode=MODE_EXEC)
    presrc_content = self.get_part_preamble(mode=MODE_EXEC,
                                            boot_code=preamble)

    f = open(src_filename, "wt")
    f.write(original_userspace)
    f.close()

    f = open(presrc_filename, "wt")
    f.write(presrc_content)
    f.close()

    box_executable = self.get_config("box_executable", "box")
    box_args = self.get_config("box_extra_args", [])
    presrc_path, presrc_basename = os.path.split(presrc_filename)

    # Command line arguments to be passed to box
    args = ["-l", "g"] + box_args

    # Include the helper code (which allows communication box-boxer)
    args += ("-I", presrc_path, "-se", presrc_basename, src_filename)

    # If the Box source is saved (rather than being a temporary unsaved
    # script) then execute it from its parent directory. Also, make sure to
    # add the same directory among the include directories. This allows
    # including scripts in the same directory.
    args += ("-I", ".")
    src_path = os.path.split(self.filename or src_filename)[0]

    # Directory from which the script should be executed
    cwd = None
    if src_path:
      cwd = src_path

    # Add extra include directories
    box_include_dirs = self.get_config("box_include_dirs", [])
    if box_include_dirs != None:
      if type(box_include_dirs) == str:
        box_include_dirs = (box_include_dirs,)
      for inc_dir in box_include_dirs:
        args += ("-I", inc_dir)

    fn = self._fns["box_document_executed"]
    def do_at_exit():
      config.tmp_remove_files(tmp_fns)
      if fn != None:
        fn(self)
      if exit_fn:
        exit_fn()

    if out_fn == None:
      out_fn = self._fns["box_exec_output"]

    return exec_command(box_executable, args,
                        out_fn=out_fn, do_at_exit=do_at_exit, cwd=cwd)

if __name__ == "__main__":
  import sys
  d = Document()
  d.load_from_file(sys.argv[1])
  sys.stdout.write("Executing...\n")
  raw_input()

  def out_fn(s):
    sys.stdout.write(s)

  d.execute(out_fn=out_fn)
  sys.stdout.write("Done\n")
  import time
  time.sleep(0.1)

