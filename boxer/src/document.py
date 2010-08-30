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

import os

import config
from exec_command import exec_command

# This is the version of the document (which may be different from the version
# of Boxer which was used to write it. We should save to file all this info,
# but we currently do not (have to improve this!)
version = (0, 1, 1)

max_chars_per_line = 79
marker_begin = "//!BOXER"
marker_sep = ":"

endline = "\n"

default_preamble = """
include "g"
GUI = Void
Window@GUI[]

"""

default_code = """
"""

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
  if not line.lstrip().startswith(marker_begin):
    return None
  else:
    return line.split(marker_sep)[1:]

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

def parse_guipoint_new(s):
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
    return (lhs.strip(), [x, y])

  except:
    return None

def parse_guipoint_v0_1_0(s):
  """Similar to parse_guipoint_new, but for Boxer 0.1."""
  try:
    lhs, rhs = s.split("=", 1)
    rem_str, _ = rhs.split(")", 1)
    rem_str, x_y_str = rem_str.split("(", 1)
    if rem_str.strip() != "":
      return None
    str_x, str_y = x_y_str.split(",", 1)
    return (lhs.strip(), [float(str_x), float(str_y)])
  except:
    return None

def parse_guipoint(s):
  """Try to parse a representation of a GUIPoint first with parse_guipoint_new
  and then with parse_guipoint_v0_1_0 if the former failed."""
  guipoint = parse_guipoint_new(s)
  if guipoint != None:
    return guipoint
  else:
    return parse_guipoint_v0_1_0(s)

class GUIPoint:
  def __init__(self, id, value=None):
    if value != None:
      self.id = id
      self.value = value
    else:
      self.id, self.value = parse_guipoint(id)

  def to_str(self, version):
    """Similar to str(guipoint), but use the proper format for the given 
    verison."""
    if version == (0, 1):
      return "%s = (%s, %s)" % (self.id, self.value[0], self.value[1])

    else:
      return "%s = Point[.x=%s, .y=%s]" \
             % (self.id, self.value[0], self.value[1])

  def __repr__(self):
    return self.to_str(version=version)

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
      guipoint_fn(line)

def save_to_str_v0_1_1(document):
  parts = document.parts
  ml_version = marker_line_assemble(["VERSION", "0", "1", "1"])
  ml_refpoints_begin = marker_line_assemble(["REFPOINTS", "BEGIN"])
  ml_refpoints_end = marker_line_assemble(["REFPOINTS", "END"])
  refpoints_text = text_writer(parts["refpoints"])
  s = (ml_version +
       parts["preamble"] + endline +
       ml_refpoints_begin + refpoints_text + ml_refpoints_end +
       parts["userspace"])
  return s

def save_to_str_v0_1(document):
  parts = document.parts
  ml_refpoints_begin = marker_line_assemble(["REFPOINTS", "BEGIN"])
  ml_refpoints_end = marker_line_assemble(["REFPOINTS", "END"])
  refpoints = [rp.to_str(version=(0, 1)) for rp in parts["refpoints"]]
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

class Document:
  def __init__(self):
    self.notifier = default_notifier
    self.attributes = {}
    self.parts = None

  def new(self, preamble=None, refpoints=[], code=None):
    if code == None:
      code = default_code
    if preamble == None:
        preamble = default_preamble
    self.parts = {
      'preamble': preamble,
      'refpoints_text': '',
      'refpoints': refpoints,
      'userspace': code
    }
    self.attributes = {
      'version': version
    }

  def get_refpoints(self):
    """Get a list of the reference points in the document (list of GUIPoint)"""
    return self.parts['refpoints']

  def get_user_part(self):
    return self.parts['userspace']

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
    marker_wants = {"REFPOINTS": 1,
                    "VERSION": 3}

    # process the file and interpret the markers
    lines = boxer_src.splitlines()
    for line in lines:
      marker = marker_line_parse(line)
      if marker == None:
        if parts.has_key(context):
          parts[context] += "\n" + line
        else:
          parts[context] = line

      else:
        if len(marker) < 1:
          raise "Internal error in Document.load_from_src"

        else:
          marker_name = marker[0]
          if not marker_wants.has_key(marker_name):
            self.notify("WARNING", "Unknown marker '%s'" % marker_name)

          else:
            if len(marker) < marker_wants[marker_name] + 1:
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

    guipoints = []
    def guipoint_fn(p_str):
      guipoints.append(GUIPoint(p_str))

    if parts.has_key("refpoints_text"):
      parse_guipoint_part(parts["refpoints_text"], guipoint_fn)
    parts["refpoints"] = guipoints

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

  def load_from_file(self, filename):
    f = open(filename, "r")
    self.load_from_str(f.read())
    f.close()

  def save_to_str(self, version=version):
    return save_to_str(self, version)

  def save_to_file(self, filename, version=version):
    f = open(filename, "w")
    f.write(self.save_to_str(version=version))
    f.close()

  def execute(self, preamble=None, out_fn=None, exit_fn=None):
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

    # In case accidentally one of such files was there already
    #config.remove_files(tmp_fns)

    box_executable = "box" #self.config.get("Box", "exec")
    presrc_path, presrc_basename = os.path.split(presrc_filename)
    extra_opts = ["-I", "."]
    if False: #self.filename != None:
      p = os.path.split(self.filename)[0]
      if len(p) > 0: extra_opts = ["-I",  p]
    args = ["-l", "g",
            "-I", presrc_path] + extra_opts + [
            "-se", presrc_basename,
            src_filename]

    def do_at_exit():
      config.tmp_remove_files(tmp_fns)
      if exit_fn:
        exit_fn()

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

