# Copyright (C) 2010-2011 Matteo Franchin (fnch@users.sf.net)
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

from docbase import \
  DocumentBase, parse_guipoint_part, text_writer, \
  refpoint_to_string, refpoint_from_string, endline

# File format version handled by this implementation of the DocumentBase class.
version = (0, 1, 1)

marker_begin = "//!BOXER"
marker_sep = ":"

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

def save_to_str_v0_1_1(document):
  ml_version = marker_line_assemble(["VERSION", "0", "1", "1"])
  ml_refpoints_begin = marker_line_assemble(["REFPOINTS", "BEGIN"])
  ml_refpoints_end = marker_line_assemble(["REFPOINTS", "END"])
  refpoints = [refpoint_to_string(rp)
               for rp in document.refpoints]
  refpoints_text = text_writer(refpoints)
  s = (ml_version +
       document.get_preamble() + endline +
       ml_refpoints_begin + refpoints_text + ml_refpoints_end +
       document.get_user_code())
  return s

def save_to_str_v0_1(document):
  ml_refpoints_begin = marker_line_assemble(["REFPOINTS", "BEGIN"])
  ml_refpoints_end = marker_line_assemble(["REFPOINTS", "END"])
  refpoints = [refpoint_to_string(rp, version=(0, 1))
               for rp in document.refpoints]
  refpoints_text = text_writer(refpoints)
  s = (document.get_preamble() + endline +
       ml_refpoints_begin + refpoints_text + ml_refpoints_end +
       document.get_user_code())
  return s

_save_to_str_fns = {(0, 1): save_to_str_v0_1,
                    (0, 1, 1): save_to_str_v0_1_1}


class Document0(DocumentBase):
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
    self.version = (0, 1, 0) # Default version

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
          raise ValueError("Internal error in Document.load_from_src")

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
    def guipoint_fn(p_str):
      refpoints.append(refpoint_from_string(p_str))

    if "refpoints_text" in parts:
      parse_guipoint_part(parts["refpoints_text"], guipoint_fn)

    if "userspace" not in parts:
      # This means that the file was not produced by Boxer, but it is likely
      # to be a plain Box file.
      parts["userspace"] = parts.get("preamble", "")
      parts["preamble"] = ""

    self.preamble = parts["preamble"]
    self.usercode = parts["userspace"]
    return True

  def save_to_str(self, version=version):
    if version in _save_to_str_fns:
      fn = _save_to_str_fns[version]
      return fn(self)

    else:
      raise ValueError("Cannot save document using version %s" % str(version))
