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

import os

from base import inherit_doc
from refpoints import RefPoint, RefPoints
import docbase
from docbase import DocumentBase, refpoint_to_string, text_writer, \
  endline
from comparse import MacroExpander, split_args, normalize_macro_name, \
  LEVEL_ERROR
from document0 import Document0


default_boot_code = \
'''include "g"
GUI = Void
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

def refpoints_from_str(s):
  """Reconstruct a RefPoints object from the string representation returned
  by the function refpoints_to_str.
  """
  pieces = s.split()
  num_points = len(pieces)/4
  rps = RefPoints()
  for i in range(num_points):
    j = 4*i
    name, svisible, sx, sy = pieces[j:j+4]
    rps.append(RefPoint(name, value=(float(sx), float(sy)),
                        visible=bool(int(svisible))))
  return rps


class BoxerMacroExpand(MacroExpander):
  """Expand Box sources to a format which can be parsed by Box."""

  def __init__(self, document=None, mode=None):
    self.document = document
    self.mode = None
    MacroExpander.__init__(self)

  def parse(self, mode=None, document=None):
    if document != None:
      self.document = document
    if mode != None:
      self.mode = mode
    return MacroExpander.parse(self, self.document.usercode)

  def macro_define_all(self, args):
    rps = filter(None, map(refpoint_to_string, self.document.refpoints))
    sep, joiner = ((",", ",".join) if self.mode == docbase.MODE_EXEC
                   else (endline, text_writer))

    return sep.join(["(**expand:define-all*)", joiner(rps), "(**end:expand*)"])

  def macro_view(self, args):
    mode = self.mode
    if mode == docbase.MODE_EXEC:
      expanded = "GUI[%s]" % args
    else:
      expanded = ""
    return "(**expand:view:%s*)%s(**end:expand*)" % (args, expanded)


(STATE_NORMAL,
 STATE_EXPAND,
 STATE_CAPTURE) = range(3)


class BoxerMacroContract(MacroExpander):
  """Contract expanded Box sources."""

  def __init__(self, *args):
    MacroExpander.__init__(self, *args)
    self.document = DocumentBase()
    self.states = [(STATE_NORMAL, None)]
    self.capture = None

  def parse(self, text=None):
    self.document.usercode = code = MacroExpander.parse(self, text)
    return code

  def push_state(self, new_state, new_context):
    self.states.append((new_state, new_context))

  def pop_state(self):
    ss = self.states
    if len(ss) > 1:
      return ss.pop()
    else:
      self.states = [(STATE_NORMAL, None)]
      return None

  def subst_source(self, content):
    s, _ = self.states[-1]
    if s == STATE_NORMAL:
      return content
    elif s == STATE_CAPTURE:
      if self.capture != None:
        self.capture += content
      return ""
    else:
      assert s == STATE_EXPAND
      return ""

  def macro_expand(self, args):
    mn = normalize_macro_name(args)
    if mn == "boxer_boot":
      self.push_state(STATE_CAPTURE, mn)
      self.capture = ""
      return ""

    else:
      self.push_state(STATE_EXPAND, mn)
      return "(**%s*)" % args

  def macro_end(self, args):
    state, context = self.states[-1]

    if len(self.states) <= 1 or state == STATE_NORMAL:
      self.notify_message(LEVEL_ERROR, "unexpected macro 'end'")
      self.pop_state() # Pop anyway...
      return ""

    else:
      assert state == STATE_EXPAND or state == STATE_CAPTURE
      mn = normalize_macro_name(args)
      if mn != 'expand':
        self.notify_message(LEVEL_ERROR,
                            "end macro expected 'expand', but got '%s'." % mn)
      self.pop_state()

      if context == "boxer_boot":
        self.document.set_boot_code(self.capture)

      return ""

  def macro_boxer_version(self, args):
    try:
      version = map(int, split_args(args))
      assert len(version) == 3

    except:
      self.notify_message(LEVEL_ERROR,
                          ("Error parsing the version number "
                           "(macro boxer-version)"))
      return None

    self.document.version = version
    return ""

  def macro_boxer_refpoints(self, args):
    try:
      rps = refpoints_from_str(args)
    except:
      self.notify_message(LEVEL_ERROR,
                          ("Error in parsing reference points "
                           "(macro boxer-refpoints)"))
      return None

    self.document.refpoints.load(rps)
    return ""


class Document1(Document0):
  def load_from_str(self, src):
    self.version = None
    mc = BoxerMacroContract(src)
    mc.document = self
    mc.parse()
    if self.version:
      return True
    else:
      success = Document0.load_from_str(self, src)
      if success:
        self.usercode = "(**define-all*)" + endline + self.usercode
        return True

      else:
        return False

  @inherit_doc
  def get_part_version(self):
    return "(**boxer-version:%d,%d,%d*)" % version

  @inherit_doc
  def get_part_boot_code(self, boot_code):
    return (boot_code or default_boot_code)

  def get_part_preamble(self, **kwargs):
    boot_code = DocumentBase.get_part_preamble(self, **kwargs)
    return endline.join(["(**expand:boxer-boot*)", boot_code,
                         "(**end:expand*)"])

  @inherit_doc
  def get_part_user_code(self, mode=None):
    """Get the user part of the Box source."""
    if mode == docbase.MODE_ORIG:
      return self.usercode

    else:
      mx = BoxerMacroExpand(document=self, mode=mode)
      output = mx.parse(mode=mode)
      if mode == docbase.MODE_EXEC:
        filename = (os.path.split(self.filename)[1]
                    if self.filename else "New file")
        return ('(**line:1,1,"%s"*)' % filename) + output

      else:
        return output

  @inherit_doc
  def get_part_def_refpoints(self):
    rps = self.get_refpoints()
    return endline.join(["(**boxer-refpoints:", refpoints_to_str(rps), "*)"])

  save_to_str = DocumentBase.save_to_str
