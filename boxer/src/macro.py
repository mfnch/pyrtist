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

from os import linesep as endline

from refpoints import RefPoint, RefPoints
from docbase import DocumentBase, refpoint_to_string, text_writer

from comparse import MacroExpander, split_args, normalize_macro_name


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
  def __init__(self, document):
    self.document = document
    MacroExpander.__init__(self)

  def parse(self, document=None):
    if document != None:
      self.document = document
    
    return MacroExpander.parse(self, self.document.usercode)

  def macro_define_all(self, args):
    rps = map(refpoint_to_string, self.document.refpoints)
    s = text_writer(rps)
    return endline.join(["(**expand:define-all*)", s, "(**end:expand*)"])

  def macro_view(self, args):
    return "(**expand:view*)GUI[%s](**end:expand*)"


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

  def notify_error(self, msg):
    print msg

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
    if mn == "boot":
      self.push_state(STATE_CAPTURE, mn)
      self.capture = ""
      return ""

    else:
      self.push_state(STATE_EXPAND, mn)
      return "(**%s*)" % args

  def macro_end(self, args):
    state, context = self.states[-1]

    if len(self.states) <= 1 or state == STATE_NORMAL:
      self.notify_error("Error: unexpected macro 'end'")
      self.pop_state() # Pop anyway...
      return ""

    else:
      assert state == STATE_EXPAND or state == STATE_CAPTURE
      mn = normalize_macro_name(args)
      if mn != 'expand':
        self.notify_error("Error: end macro expected 'expand', but got '%s'."
                          % mn)
      self.pop_state()

      if context == "boot":
        self.document.set_boot_code(self.capture)

      return ""

  def macro_boxer_version(self, args):
    try:
      version = map(int, split_args(args))
      assert len(version) == 3

    except:
      self.notify_error("Error in parsing version number(macro boxer-version)")
      return None

    self.document.version = version
    return ""

  def macro_boxer_refpoints(self, args):
    try:
      rps = refpoints_from_str(args)
    except:
      self.notify_error("Error in parsing reference points "
                        "(macro boxer-refpoints)")
      return None

    self.document.refpoints = rps
    return ""
