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

from boxer.refpoints import RefPoint, RefPoints
from boxer.document import Document, refpoint_to_string, text_writer

from parser import MacroExpander, split_args


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

  def __init__(self, document):
    self.document = document
    MacroExpander.__init__(self, document.get_user_code())

  def parse(self, **kwargs):
    self.text = self.document.get_user_code()
    return MacroExpander.parse(self, **kwargs)

  def gen_boxer_version(self):
    return "(**boxer-version:0,2,0*)"

  def gen_boxer_compat_preamble(self):
    s = self.document.parts["preamble"].strip(endline)
    return endline.join(["(**expand:boxer-preamble*)", s, "(**end-expand.*)"])

  def gen_boxer_refpoints(self):
    rps = self.document.get_refpoints()
    return endline.join(["(**boxer-refpoints:", refpoints_to_str(rps), "*)"])

  def get_output(self):
    part1 = self.gen_boxer_version()
    part2 = self.gen_boxer_compat_preamble()
    part3 = self.gen_boxer_refpoints()
    part4 = self.output
    return endline.join([part1, part2, part3, part4])

  def macro_define_all(self, args):
    rps = map(refpoint_to_string, self.document.refpoints)
    s = text_writer(rps)
    return endline.join(["(**expand:define-all*)", s, "(**end-expand.*)"])


(STATE_NORMAL,
 STATE_EXPANDING) = range(2)


class BoxerMacroContract(MacroExpander):
  """Contract expanded Box sources."""

  def __init__(self, *args):
    MacroExpander.__init__(self, *args)
    self.state = STATE_NORMAL
    self.document = Document()

  def notify_error(self, msg):
    print msg

  def invoke_method(self, name, args):
    if self.state == STATE_EXPANDING:
      self.state = STATE_NORMAL
      if name != "end_expand":
        self.notify_error("Error: expecting end-expand macro.")
      return ""

    else:
      return MacroExpander.invoke_method(self, name, args)

  def subst_source(self, content):
    if self.state != STATE_EXPANDING:
      return content
    else:
      return ""

  def macro_expand(self, args):
    self.state = STATE_EXPANDING
    return "(**%s.*)" % args

  def macro_boxer_version(self, args):
    try:
      version = map(int, split_args(args))
      assert len(version) == 3

    except:
      self.notify_error("Error in parsing version number(macro boxer-version)")
      return None

    self.document.attributes["version"] = version
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



d = Document()
d.load_from_file("./book.box")

mx = BoxerMacroExpand(d)
mx.parse()

out = mx.get_output()
print "Expanded:"
print out


print "Contracted:"
mc = BoxerMacroContract(out)
mc.parse()

#rps = d.get_refpoints()

#str_rps = refpoints_to_str(rps)
#out_rps = refpoints_from_str(str_rps)
#out_str_rps = refpoints_to_str(out_rps)

#print str_rps
#print str_rps == out_str_rps

if __name__ == "!__main__":
  text_in = \
"""
(* Program written by Me Indeed *)

(**define-all.*)

(**define-all.*)

"""

  text_expected = """

"""

  mx1 = BoxerMacroExpand(text_in)
  mx1.parse()
  text_out = mx1.get_output()
  assert text_out == text_expected, \
    ("Macro expander does not produce the expected output.")

  mx2 = BoxerMacroContract(text_out)
  mx2.parse()
  text_out = mx2.get_output()
  assert text_out == text_in, \
    ("Contracting expanded source does not lead to initial code.")
