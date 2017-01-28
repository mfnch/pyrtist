# Copyright (C) 2012, 2017 Matteo Franchin
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

import re

match_code = "``([^`]|`[^`])*``"
match_italic = "\*[^\*]+\*"
match_bold = "\*\*([^\*]|\*[^\*])*\*"

_rst_code_re = re.compile(match_code)
#_rst__re = re.compile("(%s|%s)" % (match_code, match_italic))

def parse_code(s, out=None):
  code_fragments = []
  for match in _rst_code_re.finditer(s):
    start = match.start()
    end = match.end()
    code_fragments.append((start, end, s[start+2:end-2]))

  content = (out if out != None else [])
  i = 0
  for code_fragment in code_fragments:
    start, end, code = code_fragment
    content.append(s[i:start])
    content.append((code, "code"))
    i = end
  content.append(s[i:])
  return content

def parse(content):
  out_content = []
  for piece in content:
    if type(piece) == str:
      parse_code(piece, out_content)
    else:
      out_content.append(piece)
  return out_content

if __name__ == "__main__":
  s = "here is some code ``code`` and here is **some bold text** and *some italic*. That's it!"
  print(parse_code(s))
