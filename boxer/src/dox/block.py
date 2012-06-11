# Copyright (C) 2012 Matteo Franchin
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
#   along with Boxer. If not, see <http://www.gnu.org/licenses/>.

from extractor import SLICE_COMMENT, SLICE_BLOCK, SLICE_SOURCE
from newtree import DoxSectionNode

class DoxBlock(object):
  def __init__(self, classified_slice, paragraph):
    self.classified_slice = classified_slice
    self.paragraph = paragraph
    self.target = None

  def add_node(self, tree):
    '''If required, create a Dox tree node corresponding to this block and add
    it to the tree. ``tree'' is the target Dox tree. The function returns
    '''
    return None

  def set_target(self, target):
    self.target = target


class DoxSourceBlock(DoxBlock):
  '''A block which is associated to a piece of Box source.'''

  def __init__(self, *args, **kwargs):
    DoxBlock.__init__(self, *args, **kwargs)
    self.source = None

  def get_source(self):
    '''Return the source code this documentation block refers to.
    The returned object is either a DoxTreeNode or None.'''
    return self.source

  def add_node(self, tree):
    return tree.create_node_from_source(self.get_source())

  
class DoxPreBlock(DoxSourceBlock):
  def __init__(self, *args, **kwargs):
    DoxSourceBlock.__init__(self, *args, **kwargs)
    
    # We now identify the source target, i.e. the piece of Box source code
    # that this block is documenting.
    cs = self.classified_slice
    if cs.next_source != None:
      # Get the first line of the source code which follows the doc block
      self.source = str(cs.next_source.text_slice).splitlines()[0]


class DoxPostBlock(DoxSourceBlock):
  def __init__(self, *args, **kwargs):
    DoxSourceBlock.__init__(self, *args, **kwargs)
    
    # We now identify the source target, i.e. the piece of Box source code
    # that this block is documenting.
    cs = self.classified_slice
    if cs.prev_source != None:
      # Get the last line of the source code which precedes the doc block
      self.source = str(cs.prev_source.text_slice).splitlines()[-1]


class DoxSectionBlock(DoxBlock):
  pass


class DoxIntroBlock(DoxPreBlock):
  pass


class DoxExampleBlock(DoxPreBlock):
  pass


class DoxSameBlock(DoxPostBlock):
  pass
