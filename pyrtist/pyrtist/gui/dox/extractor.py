# Copyright (C) 2012 Matteo Franchin
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

from ..comparse import TextSlice, Parser


# Slice types
(SLICE_COMMENT, # Ordinary comment: discarded
 SLICE_BLOCK,   # Documentation comment: parsed
 SLICE_SOURCE   # Source code: may be linked to documentation
) = range(3)


class ClassifiedSlice(object):
  '''A wrapper for TextSlice which adds further information: previous and next
  slice in the file and slice type.'''

  def __init__(self, text_slice, slice_type=None,
               prev_slice=None, next_slice=None):
    self.text_slice = text_slice
    self.type = slice_type
    self.prev_slice = prev_slice
    self.next_slice = next_slice
    self.prev_target = None
    self.next_target = None
    self.block = None

  def set_type(self, slice_type):
    '''Set the slice type.'''
    self.type = slice_type

  def set_prev(self, prev_slice):
    '''Add a link to the text slice which precedes this one.'''
    self.prev_slice = prev_slice

  def set_next(self, next_slice):
    '''Add a link to the text slice which follows this one.'''
    self.next_slice = next_slice

  def set_prev_target(self, prev_target):
    '''Add a link to the target (either a ClassifiedSlice or a DoxBlock) which
    precedes this block.'''
    self.prev_target = prev_target

  def set_next_target(self, next_target):
    '''Add a link to the target (either a ClassifiedSlice or a DoxBlock) which
    follows this block.'''
    self.next_target = next_target

  def set_block(self, block):
    '''Set the block (the owner of this slice).'''
    self.block = block


class DoxBlockExtractor(Parser):
  '''The role of this class is to parse Box sources to find dox comments
  and construct a list of them. This is the first step in the Dox parsing
  process. The class will construct an ordered list of TextSlice objects,
  each corresponding to a Dox comment block. The fact that we are
  collecting TextSlice objects, rather than strings reflect the will of
  keeping track of the original text. Post-processing of documentation
  blocks is done in the upper layer.

  A Dox comment block has the following form:

  (**dox comment block*)

  ///Dox comment block.
  // Second line of the comment block.

  // this is a normal comment: it does not belong to the previous block
  // because there is a blank line in between.
  '''

  def __init__(self, *args):
    self._comment = None
    self._comment_level = 0
    self.prev_block = None
    self.slices = []
    Parser.__init__(self, *args)

  def extract(self):
    '''Parse the file and return a list of ClassifiedSlice objects,
    linked together.'''
    self.parse()
    self._new_block()

    # Create the ClassifiedSlice objects
    css = []
    for slice_type, text_slice in self.slices:
      cs = ClassifiedSlice(text_slice, slice_type=slice_type)
      css.append(cs)

    # Link the objects together
    for i in range(len(css) - 1):
      csi0 = css[i]
      csi1 = css[i + 1]
      csi0.set_next(csi1)
      csi1.set_prev(csi0)

    return css

  def notify_block(self, block_type, block):
    self.slices.append((block_type, block))

  def _new_block(self, text=None):
    if self._comment != None:
      self.notify_block(SLICE_BLOCK, self._comment)
    self._comment = text

  def notify_comment_begin(self, text_slice):
    # Called by parent class on recursive comments: (* ...
    self._comment_level += 1

  def notify_comment_end(self, text_slice):
    # Called by parent class on recursive comments: ... *)
    self._comment_level -= 1
    if self._comment_level == 0:
      slice_type = (SLICE_BLOCK if text_slice.startswith("(**")
                    else SLICE_COMMENT)
      self.notify_block(slice_type, text_slice)

  def notify_inline_comment(self, text_slice):
    # Called by parent class on inline comments: // ...
    if text_slice.startswith("///"):
      self._new_block(text_slice)

    elif text_slice.startswith("//"):
      if self._comment != None:
        self._comment += text_slice

  def notify_source(self, start, end):
    # Called by parent class for blocks of source between comments.
    if start < end:
      if start >= 0 and end <= len(self.text):
        seps = self.text[start:end]
        if len(seps.strip()) > 0 or seps.count("\n") > 0:
          source_slice = TextSlice(start, None, end, text=self.text)
          self._new_block()
          self.notify_block(SLICE_SOURCE, source_slice)
