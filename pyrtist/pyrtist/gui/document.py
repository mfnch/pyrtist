# Copyright (C) 2010-2011 Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Pyrtist.
#
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

from docbase import DocumentBase, text_writer, endline, MODE_STORE
import geom2
from refpoints import RefPoint

# File format version handled by this implementation of the DocumentBase class.
version = (0, 0, 1)

marker_begin = "#!PYRTIST"
marker_sep = ":"

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

class Document(DocumentBase):
  def _get_arg_num(self, arg, possible_args):
    i = 0
    for possible_arg in possible_args:
      if possible_arg == arg:
        return i
      i += 1
    self.notify("WARNING", "unrecognized marker argument '%s'" % arg)
    return None

  def load_from_str(self, boxer_src):
    parts = {}               # different text parts of the source
    context = "preamble"     # initial context/part
    self.version = version   # Default version

    # specify how many arguments each marker wants
    marker_wants = {"CURSOR": None,
                    "REFPOINTS": 1,
                    "VERSION": 3}

    # process the file and interpret the markers
    lines = boxer_src.splitlines(True)
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
    # Obtain a list where the parents follow the children.
    children = [rp for rp in self.refpoints if not rp.is_parent()]
    parents = [rp for rp in self.refpoints if rp.is_parent()]

    if sort:
      children.sort()
      parents.sort()

    # Now add the others, which may refer to the children.
    defs = [rp.get_py_source() for rp in children + parents]
    return text_writer(defs, sep='; ').strip()

  def get_part_preamble(self, mode=None, boot_code=''):
    refpoints_part = self.get_part_def_refpoints()
    pyrtist_import = 'from pyrtist.lib2d import *'
    if mode == MODE_STORE:
      version_tokens = ["VERSION"] + [str(digit) for digit in version]
      ml_version = marker_line_assemble(version_tokens, False)
      ml_refpoints_begin = marker_line_assemble(["REFPOINTS", "BEGIN"], False)
      ml_refpoints_end = marker_line_assemble(["REFPOINTS", "END"], False)
      parts =(ml_version, pyrtist_import,
              ml_refpoints_begin, refpoints_part, ml_refpoints_end)
    else:
      parts = (pyrtist_import, boot_code, refpoints_part)
    return endline.join(parts)

  def save_to_str(self, version=version):
    return (self.get_part_preamble(mode=MODE_STORE) + endline +
            self.get_part_user_code())


if __name__ == '__main__':
  d = Document()
  d.load_from_file('/tmp/test.py')
  print(d.save_to_str())
