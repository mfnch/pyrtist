# Copyright (C) 2011 Matteo Franchin (fnch@users.sf.net)
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

import docbase
from docbase import DocumentBase, endline
from document0 import Document0
from macro import BoxerMacroContract, BoxerMacroExpand

default_boot_code = \
'''include "g"
GUI = Void
Window@GUI[]
'''

version = (0, 2, 0)


def refpoints_to_str(rps, max_line_length=79):
  """Return a compact (but human readable) representation of a RefPoints
  object (which can be mapped back to a RefPoints object using the function
  refpoints_from_str).
  """
  items = []
  for rp in rps:
    x, y = rp.value
    items.extend((str(rp.name),
                  str(int(rp.visible)),
                  repr(x),
                  repr(y)))

  text = ""
  line = None
  for item in items:
    if line == None:
      line = item

    else:
      length = len(line) + 1 + len(item)
      if length <= max_line_length:
        line += " " + item

      else:
        text += line + "\n"
        line = item

  if line != None:
    text += line
  return text


class Document1(Document0):
  def load_from_str(self, src):
    self.version = None
    mc = BoxerMacroContract(src)
    mc.document = self
    mc.parse()
    if self.version:
      return True
    else:
      return Document0.load_from_str(self, src)

  #@inherit_doc
  def get_part_version(self):
    return "(**boxer-version:%d,%d,%d*)" % version

  #@inherit_doc
  def get_part_boot_code(self, boot_code):
    code = (boot_code or default_boot_code)
    return "(**expand:boot*)%s(**end:expand*)" % code

  #@inherit_doc
  def get_part_user_code(self, mode=None):
    """Get the user part of the Box source."""
    mx = BoxerMacroExpand(self)
    if mode == docbase.MODE_STORE:
      return mx.parse()
    elif mode == docbase.MODE_EXEC:
      return mx.parse()
    else:
      return self.usercode

  #@inherit_doc
  def get_part_def_refpoints(self):
    rps = self.get_refpoints()
    return endline.join(["(**boxer-refpoints:", refpoints_to_str(rps), "*)"])

  save_to_str = DocumentBase.save_to_str
