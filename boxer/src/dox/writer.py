# Copyright (C) 2011 by Matteo Franchin (fnch@users.sourceforge.net)
#
# This file is part of Box.
#
#   Box is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Box is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Box.  If not, see <http://www.gnu.org/licenses/>.

from tree import DoxProc, DoxType


def find_closing_char(text, start=0, opening="{", closing="}"):
  n = len(text)
  n1 = n + 1

  count = 1
  i0 = start
  while i0 < n:
    i0 = min(text.find(closing, i0) % n1, text.find(opening, i0) % n1)

    if i0 == n:
      return -1

    elif i0 == start or text[i0 - 1] != "\\":
      count += 1 if text[i0] == opening else -1
      if count < 1:
        return i0

    i0 += 1

  return i0

def macro_splitter(text, start=0):
  n = len(text)
  n1 = n + 1

  i0 = start
  pieces = []
  while i0 < n:
    i1 = text.find("{", i0)

    if i1 < 0:
      pieces.append(text[i0:])
      return pieces

    elif i1 == start or text[i1 - 1] != "\\":
      pieces.append(text[i0:i1])
      i0 = i1 + 1
      i1 = find_closing_char(text, i0) % n1
      pieces.append(text[i0:i1])

    i0 = i1 + 1

  return pieces



class Writer(object):
  def __init__(self, tree, docinfo={}, ext=None):
    self.tree = tree
    self.docinfo = docinfo
    self.default_extension = ext

  def gen_type_link(self, t):
    return "link(%s)" % str(t)

  def gen_target_section(self, target, section="Intro"):
    if target != None:
      db = target.doxblocks
      if db != None and section in db.content:
        text = " ".join(db.content[section])
        pieces = macro_splitter(text)

        expanded_pieces = []
        for nr_piece, piece in enumerate(pieces):
          is_macro = nr_piece % 2
          resulting_piece = piece
          if is_macro:
            interpreted = self.macro_interpreter(target, piece)
            if interpreted != None:
              resulting_piece = interpreted
          expanded_pieces.append(resulting_piece)
        return expanded_pieces

    return None

  def macro_interpreter(self, target, text, start=0):
    st = text.strip()
    if st.lower().startswith("type"):
      arg = st[4:].lstrip()
      if arg[0] == "(" and arg[-1] == ")":
        arg = arg[1:-1].strip()

        if arg in [".@", "@."] and isinstance(target, DoxProc):
          t = (target.child if arg == ".@" else target.parent)

        elif arg == "." and isinstance(target, DoxType):
          t = target

        else:
          t = self.tree.get_type(arg)

        return self.gen_type_link(t)

      else:
        return None