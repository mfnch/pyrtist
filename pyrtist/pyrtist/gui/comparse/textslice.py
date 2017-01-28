# Copyright (C) 2012 Matteo Franchin (fnch@users.sourceforge.net)
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


def select_one(*args):
  '''Return the argument which is not None and raise an error if more
  than one argument was not None. Examples:
    select_one(None, 2, None) --> 2
    select_one() --> None
    select_one(None) --> None
    select_one(1, None, None, 2) --> raise error
  '''
  nnone_args = [arg for arg in args if arg != None]
  if len(nnone_args) > 1:
    raise ValueError("Conflicting attributes: %s and %s"
                     % tuple(nnone_args[:2]))
  return nnone_args[0] if len(nnone_args) == 1 else None


class TextSlice(object):
  '''A TextSlice object represents a portion of text. It includes the
  initial and final character and the line number.'''

  def __init__(self, begin, lineno, end=None, parent=None, text=None):
    self.begin = begin
    self.end = end
    self.lineno = lineno
    self.parent = parent
    self.text = text

  def __str__(self):
    return self.text[self.begin:self.end]

  def __add__(self, rhs):
    begin = min(self.begin, rhs.begin)
    end = max(self.end, rhs.end)
    lineno = min(self.lineno, rhs.lineno)
    assert self.text == rhs.text, "The slices are referring to different text"
    return TextSlice(begin, lineno, end, text=self.text)

  def __getitem__(self, idx):
    if type(idx) == int:
      idx = slice(idx, idx + 1)

    assert idx.step in [None, 1], "Slices are not supported"

    begin = self.begin
    end = self.end

    if idx.start != None:
      begin += idx.start
    if idx.stop != None:
      end = self.begin + idx.stop

    lineno = (self.lineno
              + re_count(endline_re, self.text, self.begin, begin))
    return TextSlice(begin, lineno, end, text=self.text)

  def startswith(self, *args):
    return str(self).startswith(*args)
