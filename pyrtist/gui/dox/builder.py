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


# Introduction to the Box documentation system and how it works
# =============================================================
#
# During the Dox parse process, the documentation in a selected set of Box
# source files is read and the documentation tree is generated.  The
# documentation tree (DoxTree in file dox/tree.py) is a representation of the
# documentation and can be used to generate the final output. The entire
# documentation process in Dox can thus be subdivided in two phases:
#
# 1) a set of files is read and parsed and a documentation tree is generated,
#
# 2) the documentation tree is translated into one of the available output
#    formats.
#
# We now enter the details of the parsing process (phase 1 above).
#
# PHASE 1: Parsing of documentation and generation of the parse tree
# ------------------------------------------------------------------
#
# The parsing itself is subdivided into different stages.
#
# 1) An empty DoxTree object is created,
#
# 2) Each file in the set of input source files is opened in sequence,
#
# 3) The file is subdivided in text slices of different types: source slices,
#    comment slices, documentation blocks (comments starting with (** or ///),
#
# 4) The slices are concatenated in an ordered list. Each text slice is made
#    aware of what text slices are preceding and following it.
#
# 5) Slices are interpreted. Documentation slices are mapped to DoxBlock
#    objects. Each block is linked to its originating text slice,
#
# 6) Each documentation block is associated to a target (i.e. whatever the
#    block is documenting). Blocks may be themselves documentation targets
#    for other blocks.
#
# 7) Each block is given an opportunity to inspect the current context and
#    modify it, if necessary. The concept of context is mainly used to
#    organize nodes and put them into sections.
#
# 8) DoxBlock objects are given an opportunity to generate a node of the tree.
#    Basically, the method DoxBlock.add_node is called and the tree is passed
#    as an argument. The block can thus manipulate the tree.
#    Nodes are linked to their blocks and blocks are linked to their nodes.
#
# 9) After doing all this for each source file, the DoxTree.process method is
#    called. This method does global processing of the tree (while what we have
#    seen so far was per-file processing). Missing types are detected, subtypes
#    are associated to their parent types, etc.
#
# The tree is now complete. It contains all the information we need to write an
# HTML file with the documentation or to populate the controls of the Dox
# browser.

import sys

from extractor import DoxBlockExtractor, SLICE_BLOCK, SLICE_SOURCE
from block import get_block_lines, split_block, DoxIntroBlock, \
  DoxExampleBlock, DoxSameBlock, DoxSectionBlock, DoxPreviewBlock
from logger import log_msg


known_block_types = \
  {'section': DoxSectionBlock,
   'intro': DoxIntroBlock,
   'example': DoxExampleBlock,
   'same': DoxSameBlock,
   'preview': DoxPreviewBlock}

def create_blocks_from_classified_slices(classified_slices):
  '''Create documentation block objects from the classified text slices.
  In this phase, text slices are parsed and mapped to DoxBlock objects of
  different kinds (``Intro'' blocks are mapped to ``DoxIntro'' blocks, etc).'''

  # Get the text from the slices
  if len(classified_slices) > 0:
    text = classified_slices[0].text_slice.text

  for cs in classified_slices:
    if cs.text_slice.text != text:
      raise ValueError("Slices are referring to different texts")

  # For each block construct an appropriate DoxBlock object
  doxblocks = []
  for cs in classified_slices:
    if cs.type == SLICE_BLOCK:
      lines = get_block_lines(str(cs.text_slice))
      if not lines:
        log_msg('Invalid documentation block')
        continue

      paragraph = " ".join(lines).strip()
      block_type, content = split_block(paragraph)
      if not (block_type and content):
        log_msg('Missing documentation identifier')
        continue

      block_type = block_type.lower()

      target_on_left = block_type.startswith('<')
      if target_on_left:
        block_type = block_type[1:]
        
      constructor = known_block_types.get(block_type)
      if constructor != None:
        # Call the constructor for the block
        doxblock = constructor(cs, content, target_on_left=target_on_left)
        cs.set_block(doxblock)
        doxblocks.append(doxblock)

      else:
        log_msg("Unrecognized block '%s'" % block_type)

  return doxblocks

def add_blocks_to_tree(tree, blocks):
  '''Add the nodes associated to the given blocks to the given tree.
  This is the last phase of the documentation parsing process, in which the
  various nodes of the tree are built and added to the tree.'''

  # Now create the nodes for the various blocks: first deal with target blocks,
  # then do the others.
  for do_targets in (True, False):
    for block in blocks:
      if block.is_target == do_targets:
        node = block.add_node(tree)
        if node != None:
          node.add_block(block)
          block.set_target(node)

def create_classified_slices_from_text(text):
  '''Split the text into an ordered and linked list of classified text slices
  (i.e. ClassifiedSlice objects). This is the first step of the documentation
  parsing process.'''
  dbe = DoxBlockExtractor(text)
  return dbe.extract()

def associate_targets_to_blocks(classified_slices):
  '''Carry out association of blocks to their targets.
  This step is necessary in order to ensure that, for example, an ``Intro''
  block preceding a procedure or type definition is associated with it.'''

  # For each block, identify (and communicate to it) what the closest following
  # target is.
  next_target = None
  for cs in reversed(classified_slices):
    if cs.type == SLICE_SOURCE:
      next_target = cs

    elif cs.type == SLICE_BLOCK:
      block = cs.block
      if block:
        if block.is_target:
          next_target = block

        else:
          block.classified_slice.set_next_target(next_target)

  # For each block, identify (and communicate to it) what the closest preceding
  # target is.
  prev_target = None
  for cs in classified_slices:
    if cs.type == SLICE_SOURCE:
      prev_target = cs

    elif cs.type == SLICE_BLOCK:
      block = cs.block
      if block:
        if block.is_target:
          prev_target = block

        else:
          block.classified_slice.set_prev_target(prev_target)

def associate_contexts_to_blocks(blocks, initial_context):
  '''This steps associates each block to the corresponding context.
  This step is necessary in order to place types, instances, ... into the
  appropriate section, for example.'''
  context = initial_context
  for block in blocks:
    context = block.use_context(context)
  return context


if __name__ == '__main__':
  import rst
  from tree import DoxTree 
  with open(sys.argv[1], "r") as f:
    text = f.read()

  tree = DoxTree()
  slices = create_classified_slices_from_text(text)
  blocks = create_blocks_from_classified_slices(slices)
  add_blocks_to_tree(tree, blocks)

  tree.process()

  writer = rst.RSTWriter(tree)
  writer.save()
