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

import sys
import os

import config
from config import Configurable
from exec_command import exec_command
from refpoints import RefPoint, RefPoints

# This is the version of the document (which may be different from the version
# of Boxer which was used to write it. We should save to file all this info,
# but we currently do not (have to improve this!)
version = (0, 1, 1)

max_chars_per_line = 79
marker_begin = "//!BOXER"
marker_sep = ":"

endline = "\n"

default_preamble = 'include "g"\nGUI = Void\nWindow@GUI[]\n\n'

default_code = "\n"

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

def marker_line_parse(line):
  """Extract the arguments of a marker line or return None if the given
  string is not a marker line."""
  if not line.strip().startswith(marker_begin):
    return None
  else:
    return [s.strip() for s in line.split(marker_sep)[1:]]

def marker_line_assemble(attrs, newline=True):
  """Crete a marker line from the given list of attributes."""
  s = marker_sep.join([marker_begin] + attrs)
  if newline:
    s += endline
  return s

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
    print "original '%s' err: %s" % (s, x)
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

  else:
    return ("%s = Point[.x=%s, .y=%s]"
            % (rp.name, rp.value[0], rp.value[1]))

def default_notifier(level, msg):
  print "%s: %s" % (level, msg)

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

def save_to_str_v0_1_1(document):
  parts = document.parts
  ml_version = marker_line_assemble(["VERSION", "0", "1", "1"])
  ml_refpoints_begin = marker_line_assemble(["REFPOINTS", "BEGIN"])
  ml_refpoints_end = marker_line_assemble(["REFPOINTS", "END"])
  refpoints = [refpoint_to_string(rp)
               for rp in document.refpoints]
  refpoints_text = text_writer(refpoints)
  s = (ml_version +
       parts["preamble"] + endline +
       ml_refpoints_begin + refpoints_text + ml_refpoints_end +
       parts["userspace"])
  return s

def save_to_str_v0_1(document):
  parts = document.parts
  ml_refpoints_begin = marker_line_assemble(["REFPOINTS", "BEGIN"])
  ml_refpoints_end = marker_line_assemble(["REFPOINTS", "END"])
  refpoints = [refpoint_to_string(rp, version=(0, 1))
               for rp in document.refpoints]
  refpoints_text = text_writer(refpoints)
  s = (parts["preamble"] + endline +
       ml_refpoints_begin + refpoints_text + ml_refpoints_end +
       parts["userspace"])
  return s

_save_to_str_fns = {(0, 1): save_to_str_v0_1,
                    (0, 1, 1): save_to_str_v0_1_1}

def save_to_str(document, version=version):
  if _save_to_str_fns.has_key(version):
    fn = _save_to_str_fns[version]
    return fn(document)

  else:
    raise ValueError("Cannot save document using version %s" % str(version))

class Document(Configurable):
  """Class to load, save, display and edit Boxer documents.
  A Boxer document is basically a Box program plus some metainfo contained
  in comments."""

  def __init__(self, filename=None, callbacks=None,
               config=None, from_args=None):
    """filename=None :the filename associated to the document."""
    Configurable.__init__(self, config=config, from_args=from_args)
    self.notifier = default_notifier
    self.attributes = {}
    self.parts = None
    self.filename = None
    if callbacks == None:
      callbacks = {}
    callbacks.setdefault("box_document_execute", None)
    callbacks.setdefault("box_document_executed", None)
    callbacks.setdefault("box_exec_output", None)
    self._fns = callbacks
    self.refpoints = RefPoints(callbacks=self._fns)
    self.notify = lambda t, msg: sys.stdout.write("%s: %s\n" % (t, msg))

  def new(self, preamble=None, refpoints=[], code=None):
    if code == None:
      code = default_code
    if preamble == None:
        preamble = default_preamble
    self.refpoints.load(refpoints)
    self.parts = {'preamble': preamble,
                  'refpoints_text': '',
                  'userspace': code}
    self.attributes = {'version': version}

  def get_refpoints(self):
    """Get a list of the reference points in the document (list of GUIPoint)"""
    return self.refpoints

  def get_user_part(self):
    return self.parts['userspace']

  get_user_code = get_user_part

  def set_user_code(self, code):
    self.parts['userspace'] = code

  def _get_arg_num(self, arg, possible_args):
    i = 0
    for possible_arg in possible_args:
      if possible_arg == arg:
        return i
      i += 1
    self.notify("WARNING", "unrecognized marker argument '%s'" % arg)
    return None

  def load_from_str(self, boxer_src):
    parts = {}           # different text parts of the source
    context = "preamble" # initial context/part

    # default attributes
    self.attributes["version"] = (0, 1, 0)

    # specify how many arguments each marker wants
    marker_wants = {"CURSOR": None,
                    "REFPOINTS": 1,
                    "VERSION": 3}

    # process the file and interpret the markers
    lines = boxer_src.splitlines(True)
    for line in lines:
      marker = marker_line_parse(line)
      if marker == None:
        if context in parts:
          parts[context] += line
        else:
          parts[context] = line

      else:
        if len(marker) < 1:
          raise "Internal error in Document.load_from_src"

        else:
          marker_name = marker[0]
          if not marker_name in marker_wants:
            self.notify("WARNING", "Unknown marker '%s'" % marker_name)

          else:
            marker_nargs = marker_wants[marker_name]
            if marker_nargs != None and len(marker) < marker_nargs + 1:
              self.notify("WARNING",
                          "Marker has less arguments than expected")

            elif marker_name == "REFPOINTS":
              arg_num = self._get_arg_num(marker[1], ["BEGIN", "END"])
              if arg_num == None:
                return False
              else:
                context = ["refpoints_text", "userspace"][arg_num]

            elif marker_name == "VERSION":
              try:
                self.attributes["version"] = \
                  (int(marker[1]), int(marker[2]), int(marker[3]))
              except:
                self.notify("WARNING", "Cannot determine Boxer version which "
                            "generated the file")

            else:
              parts[context] += line
              # ^^^  note that this requires context to already exist in the
              #      dictionary. In other words, unrecognized markers cannot
              #      be the first markers in the file.

    refpoints = self.refpoints
    refpoints.remove_all()
    def guipoint_fn(p_str):
      refpoints.append(refpoint_from_string(p_str))

    if parts.has_key("refpoints_text"):
      parse_guipoint_part(parts["refpoints_text"], guipoint_fn)

    if not parts.has_key("userspace"):
      # This means that the file was not produced by Boxer, but it is likely
      # to be a plain Box file.
      if parts.has_key("preamble"):
        parts["userspace"] = parts["preamble"]
      else:
        parts["userspace"] = ""
      parts["preamble"] = ""

    self.parts = parts
    return True

  def load_from_file(self, filename, remember_filename=True):
    f = open(filename, "r")
    self.load_from_str(f.read())
    f.close()

    if remember_filename:
      self.filename = filename

  def save_to_str(self, version=version):
    return save_to_str(self, version)

  def save_to_file(self, filename, version=version, remember_filename=True):
    f = open(filename, "w")
    f.write(self.save_to_str(version=version))
    f.close()

    if remember_filename:
      self.filename = filename

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

    original_preamble = self.parts["preamble"]
    original_userspace = self.parts["userspace"]
    try:
      if preamble != None:
        self.parts["preamble"] = preamble
      self.parts["userspace"] = ""
      presrc_content = self.save_to_str()
    except Exception as the_exception:
      raise the_exception
    finally:
      self.parts["preamble"] = original_preamble
      self.parts["userspace"] = original_userspace

    f = open(src_filename, "wt")
    f.write(original_userspace)
    f.close()

    f = open(presrc_filename, "wt")
    f.write(presrc_content)
    f.close()

    box_executable = self.get_config("box_executable", "box")
    box_args = self.get_config("box_extra_args", [])
    presrc_path, presrc_basename = os.path.split(presrc_filename)
    extra_opts = ["-I", "."]
    if self.filename != None:
      p = os.path.split(self.filename)[0]
      if len(p) > 0: extra_opts = ["-I",  p]
    args = ["-l", "g",
            "-I", presrc_path] + extra_opts + [
            "-se", presrc_basename,
            src_filename] + box_args

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
                        out_fn=out_fn, do_at_exit=do_at_exit)

if __name__ == "__main__":
  import sys
  d = Document()
  d.load_from_file(sys.argv[1])
  print "Executing..."
  raw_input()

  def out_fn(s):
    print s,

  d.execute(out_fn=out_fn)
  print "Done"
  import time
  time.sleep(0.1)

