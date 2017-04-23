# Copyright (C) 2010-2017 Matteo Franchin (fnch@users.sf.net)
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

import sys
import os
import itertools

from . import geom2
from .config import Configurable
from .scriptrunner import run_script
from .refpoints import RefPoint, RefPoints
from .callbacks import Callbacks

# This is the version of the document (which may be different from the version
# of Boxer which was used to write it. We should save to file all this info,
# but we currently do not (have to improve this!)
version = (0, 0, 1)

max_chars_per_line = 79
marker_begin = "#!PYRTIST"
marker_sep = ":"
marker_cursor_here = "#!PYRTIST:CURSOR:HERE"

#from os import linesep as endline
endline = "\n"

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

def marker_line_parse(line):
  """Extract the arguments of a marker line or return None if the given
  string is not a marker line."""
  if not line.strip().startswith(marker_begin):
    return None
  return [s.strip() for s in line.split(marker_sep)[1:]]

def marker_line_assemble(attrs, newline=True):
  """Crete a marker line from the given list of attributes."""
  s = marker_sep.join([marker_begin] + attrs)
  if newline:
    s += endline
  return s


class Document(Configurable):
  '''Class to load, save, display and edit Pyrtist documents.
  A Pyrtist document is basically a Python program plus some metainfo contained
  in comments.'''

  def __init__(self, filename=None, callbacks=None,
               config=None, from_args=None):
    """filename=None :the filename associated to the document."""
    super(Document, self).__init__(config=config, from_args=from_args)

    self.filename = None         # File name corresponding to this document
    self.version = None          # Version of the file format
    self.preamble = None         # Preamble of the document
    self.usercode = None         # Body of the document

    self.callbacks = cbs = Callbacks.share(callbacks)
    cbs.default("box_document_execute")
    cbs.default("box_document_executed")

    self.refpoints = RefPoints(callbacks=cbs)

  def notify(self, t, msg):
    '''Used to notify problems during load/save.'''
    sys.stdout.write("%s: %s\n" % (t, msg))

  def new(self):
    '''Start working on a new document (un-named).'''
    self.filename = None

  def get_refpoints(self):
    '''Get a list of the reference points in the document (list of GUIPoint)'''
    return self.refpoints

  def set_user_code(self, code):
    '''Set the content of the user-provided part of the Python script.'''
    self.usercode = code

  def get_part_user_code(self):
    '''Get the user-provided part of the Python script.'''
    return self.usercode

  def load_from_file(self, filename):
    with open(filename, "r") as f:
      self.load_from_str(f.read())
    self.filename = filename

  def save_to_file(self, filename, version=version, remember_filename=True):
    with open(filename, "w") as f:
      f.write(self.save_to_str(version=version))

    if remember_filename:
      self.filename = filename

  def _get_arg_num(self, arg, possible_args):
    i = 0
    for possible_arg in possible_args:
      if possible_arg == arg:
        return i
      i += 1
    self.notify("WARNING", "unrecognized marker argument '%s'" % arg)
    return None

  def load_from_str(self, py_src):
    parts = {}               # different text parts of the source
    context = "preamble"     # initial context/part
    self.version = version   # Default version

    # specify how many arguments each marker wants
    marker_wants = {"CURSOR": None,
                    "REFPOINTS": 1,
                    "VERSION": 3}

    # process the file and interpret the markers
    lines = py_src.splitlines(True)
    for line in lines:
      marker = marker_line_parse(line)
      if marker is None:
        if context in parts:
          parts[context] += line
        else:
          parts[context] = line
        continue

      if len(marker) < 1:
        raise ValueError("Internal error in Document.load_from_src")

      marker_name = marker[0]
      if not marker_name in marker_wants:
        self.notify("WARNING", "Unknown marker '%s'" % marker_name)
        continue

      marker_nargs = marker_wants[marker_name]
      if marker_nargs is not None and len(marker) < marker_nargs + 1:
        self.notify("WARNING",
                    "Marker has less arguments than expected")
      elif marker_name == "REFPOINTS":
        arg_num = self._get_arg_num(marker[1], ["BEGIN", "END"])
        if arg_num is None:
          return False
        context = ["refpoints_text", "userspace"][arg_num]
      elif marker_name == "VERSION":
        try:
          assert len(marker) == 4
          self.version = map(int, marker[1:])
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

    if "refpoints_text" in parts:
      context = {"Point": geom2.Point, "Tri": geom2.Tri}
      exec(parts["refpoints_text"], context)
      points_by_id = {}
      for name, value in context.items():
        if isinstance(value, geom2.Point):
          points_by_id[id(value)] = rp = RefPoint(name, list(value))
          refpoints.append(rp)
      for name, value in context.items():
        if isinstance(value, geom2.Tri):
          tri_args = list(value)
          if len(tri_args) < 2:
            # Weird: Tri() object is degenerate. Load it as a Point().
            if len(tri_args) == 1:
              refpoints.append(RefPoint(name, list(tri_args[0])))
          else:
            rp_parent = RefPoint(name, list(tri_args.pop(1)))
            refpoints.append(rp_parent)
            rp_children = [points_by_id.get(id(arg)) for arg in tri_args]
            for idx, rp_child in enumerate(rp_children):
              rp_parent.attach(rp_child, index=idx)

    if "userspace" not in parts:
      # This means that the file was not produced by Boxer, but it is likely
      # to be a plain Box file.
      parts["userspace"] = parts.get("preamble", "")
      parts["preamble"] = ""

    self.preamble = parts["preamble"]
    self.usercode = parts["userspace"]
    return True

  def get_part_def_refpoints(self, sort=True):
    '''Produce the part of the file which stores information about the
    reference points.
    '''
    # Obtain a list where the parents follow the children.
    children = [rp for rp in self.refpoints if not rp.is_parent()]
    parents = [rp for rp in self.refpoints if rp.is_parent()]

    if sort:
      children.sort()
      parents.sort()

    # Now add the others, which may refer to the children.
    defs = [rp.get_py_source() for rp in itertools.chain(children, parents)]
    return text_writer(defs, sep='; ').strip()

  def get_part_preamble(self, mode=None):
    '''Return the first part of the Pyrtist script, where meta-information such
    as the version of the file format and the reference points are defined.
    '''
    refpoints_part = self.get_part_def_refpoints()
    gui_import = 'from pyrtist.lib2d import Point, Tri'
    version_tokens = ["VERSION"] + [str(digit) for digit in version]
    ml_version = marker_line_assemble(version_tokens, False)
    ml_refpoints_begin = marker_line_assemble(["REFPOINTS", "BEGIN"], False)
    ml_refpoints_end = marker_line_assemble(["REFPOINTS", "END"], False)
    parts =(ml_version, gui_import,
            ml_refpoints_begin, refpoints_part, ml_refpoints_end)
    return endline.join(parts)

  def save_to_str(self, version=version):
    return self.get_part_preamble() + endline + self.get_part_user_code()

  @staticmethod
  def build_refpoint_set_cmds(tag, refpoints):
    '''Transform a list of refpoints `refpoints` to commands that can be sent
    to the script to set its environment consistently. The `tag` string can
    be either "new" or "old" to specify whether `refpoints` is providing the
    current value of the reference points or the value before the last user
    modification (e.g. dragging). Knowing which reference points were last
    modified and what was their old value offers the script the opportunity
    of responding better to user actions.
    '''
    # We ensure Tri points are generated after their children by appending the
    # `new_tri' commands after all the `new_point' commands. This is done by
    # using two separate lists that are then merged.
    point_cmds = []
    tri_cmds = []
    for rp in refpoints:
      x, y = rp.value
      if rp.is_parent():
        cld = rp.get_children()
        lhs_name = (cld[0].name if len(cld) >= 1 and cld[0] is not None
                    else None)
        rhs_name = (cld[1].name if len(cld) >= 2 and cld[1] is not None
                    else None)
        tri_cmds.append(('tri', tag, rp.name, x, y, lhs_name, rhs_name))
      else:
        point_cmds.append(('point', tag, rp.name, x, y))
    point_cmds.extend(tri_cmds)
    return point_cmds

  def execute(self, callback=None, startup_cmds=None):
    self.callbacks.call("box_document_execute", self)

    # Pass the point names and coordinates to the GUI gate object.
    cmds = self.build_refpoint_set_cmds('new', self.refpoints)
    cmds.extend(startup_cmds)

    # If the Box source is saved (rather than being a temporary unsaved
    # script) then execute it from its parent directory. Also, make sure to
    # add the same directory among the include directories. This allows
    # including scripts in the same directory.
    cwd = None
    src_name = '<New file>'
    if self.filename is not None:
      cwd = os.path.split(self.filename)[0]
      # Put the file name between angle brackets to prevent the backtrace
      # formatter from trying to load the file when showing where the error
      # is. This would not work as the script we are executing is not exactly
      # the same as the one which sits on the disk.
      src_name = '<{}>'.format(self.filename)

    def my_callback(name, *args):
      self.callbacks.call(name, *args)
      if name == 'exit':
        self.callbacks.call('box_document_executed', self)
      if callback:
        callback(name, *args)

    src = self.get_part_user_code()
    return run_script(src_name, src, callback=my_callback, cwd=cwd,
                      startup_cmds=cmds)


if __name__ == '__main__':
  d = Document()
  d.load_from_file('/tmp/test.py')
  print(d.save_to_str())
