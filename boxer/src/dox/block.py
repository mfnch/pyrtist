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

import copy

from extractor import \
  SLICE_COMMENT, SLICE_BLOCK, SLICE_SOURCE, ClassifiedSlice
from tree import DoxSectionNode
from logger import log_msg


class DoxBlock(object):
  # Name of the block (intro, section, etc.).
  block_name = None

  # Whether many blocks of this type can be associated to the same target.
  multiple_allowed = False

  # Is the block itself a target for other blocks.
  is_target = False

  def __init__(self, classified_slice, content, target_on_left=False):
    self.target_on_left = target_on_left
    self.classified_slice = classified_slice
    self.content = content
    self.target = None
    self.context = None

  def use_context(self, context):
    '''Called with the current 'Context'. This method is expected to change the
    current context, if required, and associate the block to the current
    context. The changed context is returned.'''
    self.context = context
    return context

  def get_content(self):
    '''Get the text associated with the block.'''
    return self.content

  def add_node(self, tree):
    '''If required, create a Dox tree node corresponding to this block and add
    it to the tree. ``tree'' is the target Dox tree. The function returns
    '''
    return None

  def set_target(self, target):
    '''Set the target tree node associated to this block.'''
    self.target = target

  def get_target(self):
    '''Get the target tree node associated to this block (None if missing).'''
    return self.target


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
    '''See DoxBlock.add_node'''

    # We now identify the target, i.e. whatever this block is documenting.
    cs = self.classified_slice
    target = (cs.prev_target if self.target_on_left else cs.next_target)

    if target != None:
      if isinstance(target, ClassifiedSlice):
        # This block is documenting the piece of Box source code which
        # follows/precedes: get the first line of it.
        self.source = source = \
          str(target.text_slice).splitlines()[-1 if self.target_on_left else 0]
        node = tree.create_node_from_source(source)

        # We here associate the node with its parent section.
        # We also add the node to it.
        if node:
          sectionblock = self.context.get('section', None)
          if sectionblock:
            sectionnode = sectionblock.get_target()
            node.set_section(sectionnode)
            sectionnode.add_node(node)

        return node

      else:
        # This block is documenting another block (e.g. a ``Section'' block).
        assert isinstance(target, DoxBlock) and target.is_target, \
          'Block targets invalid destination block'
        # The target block has been created by now, so we just need to return
        # it.
        return target.target

  
class DoxSectionBlock(DoxBlock):
  block_name = 'section'
  is_target = True

  def add_node(self, tree, *args, **kwargs):
    '''See DoxBlock.add_node'''

    # Register the section in the tree
    section_path = self.content
    root_section = tree.get_root_section()
    return root_section.get_subsection(section_path, create=True)

  def use_context(self, context):
    context = context.create_context(section=self)
    return DoxBlock.use_context(self, context)


class DoxIntroBlock(DoxSourceBlock):
  block_name = 'intro'


class DoxExampleBlock(DoxSourceBlock):
  block_name = 'example'
  multiple_allowed = True
  multiple_allowed = False # This is not yet supported...


class DoxSameBlock(DoxSourceBlock):
  '''A ``Same'' block copies the documentation blocks from the last target.'''
  block_name = 'same'

  def add_node(self, tree):
    '''See DoxBlock.add_node'''
    node = DoxSourceBlock.add_node(self, tree)
    if node == None:
      return None

    # We first try to find the first non-source slice that precedes ours.
    cs = self.classified_slice
    while cs != None:
      cs = cs.prev_slice
      if cs.type == SLICE_BLOCK:
        if cs.block and cs.block.target:
          break

        else:
          log_msg("Cannot find target for 'Same' block")
          return None
    
    # We have the previous target: we now steal stuff from it.
    source_node = cs.block.target
    for block in source_node.get_blocks():
      node.add_block(copy.copy(block))

    return node
