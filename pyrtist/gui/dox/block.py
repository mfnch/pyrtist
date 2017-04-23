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

import copy

from extractor import \
  SLICE_COMMENT, SLICE_BLOCK, SLICE_SOURCE, ClassifiedSlice
from tree import DoxSectionNode
from logger import log_msg


def get_block_lines(text):
  '''Return the lines in a doxblock independenlty of the actual format of the
  documentation block. This involves stripping the comment delimiters and
  removing the newline characters. A list of lines is returned, if the
  operation succeeds, None is returned if the operation fails.'''

  if len(text) >= 5 and text.startswith("(**") and text.endswith("*)"):
    return text[3:-2].splitlines()

  elif text.startswith("///"):
    return [line[2:] for line in text[1:].splitlines()]

  return None

def split_block(doxblock_content):
  '''Takes the content of a documentation block and split the block
  identification keyword and the content returning. Below is an example.
  Each documentation block starts with a block identifier. For example,

  ///Intro: a brief
  // introduction.

  This function takes the string 'Intro: a brief introduction.' and returns
  a tuple ('Intro', 'a brief introduction.') or (None, None) if the split
  was not possible.'''

  args = tuple(item for item in map(doxblock_content.find, (":", "."))
               if item >= 0)
  if not args:
    return (None, None)

  idx = min(args)
  if idx == None:
    return (None, None)

  block_type = doxblock_content[:idx].strip()
  block_content = doxblock_content[idx + 1:].strip()
  return (block_type, block_content) 


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


class DoxPreviewBlock(DoxSourceBlock):
  '''Block used to include a piece of Box source code. The block allows the
  user to control the format (placing of the newlines) independently of the
  actual format of the comment. Example:

  (**xxxx: |// Line 1 of source |  // Line 2
           |some_expr = some_other_expr *)

  Which gives:

  // Line 1 of source 
    // Line 2
  some_expr = some_other_expr *)
  '''

  block_name = 'preview'

  def __init__(self, *args, **kwargs):
    DoxSourceBlock.__init__(self, *args, **kwargs)
    self.code = self._reformat_code()

  def _reformat_code(self):
    text_slice = self.classified_slice.text_slice
    assert text_slice

    # We get the lines, we then try to determine the line separator
    lines = get_block_lines(str(text_slice).strip())
    if not lines:
      return None

    first_line = split_block(lines[0])[1].lstrip()
    if first_line:
      # If the first line is not empty, then it starts with the delimiter
      # (trailing spaces are ignored).
      # Example: '(**Preview: |  Line1|    Line2 | Line3  *)' will expand to
      # 'Line1\n   Line2 \n Line3  '
      sep = first_line[0]

      # Join all the lines and split them with the specified delimiter
      content = (first_line[1:].lstrip() +
                 ''.join(map(str.lstrip, lines)[1:]))
      return content.replace(sep, '\n')

    else:
      # If the first line is empty (nothing other than spaces follow the
      # identifier), then the text is returned as given, with end of lines
      # appearing exactly where they appear on the comment
      return '\n'.join(lines[1:])

  def get_code(self):
    '''Return the reformatted code contained in this block.'''
    return self.code
