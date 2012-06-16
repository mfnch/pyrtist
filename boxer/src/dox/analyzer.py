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
#    aware of what text slices are preceding and following it and what source
#    slices are preceding and following it. The latter is useful as it allows
#    each documentation block to know what part of the Box source it may be
#    referring to,
#
# 5) Slices are interpreted. Documentation slices are mapped to DoxBlock
#    objects. Each block is linked to its originating text slice,
#
# 6) DoxBlock objects are given an opportunity to generate a node of the tree.
#    Basically, the method DoxBlock.add_node is called and the tree is passed
#    as an argument. The block can thus manipulate the tree.
#    Nodes are linked to their blocks and blocks are linked to their nodes.
#
# 7) The blocks are iterated again. This time they are given an opportunity to
#    modify the context. The concept of context is mainly used to organize
#    nodes and put them into sections.
#
# 8) After doing all this for each source file, the DoxTree.process method is
#    called. This method does global processing of the tree (while what we have
#    seen so far was per-file processing). Missing types are detected, subtypes
#    are associated to their parent types, etc.
#
# The tree is now complete. It contains all the information we need to write an
# HTML file with the documentation or to populate the controls of the Dox
# browser.

import sys

from extractor import DoxBlockExtractor, SLICE_BLOCK
from block import DoxIntroBlock, DoxExampleBlock, DoxSameBlock, DoxSectionBlock
from logger import log_msg


def split_block(doxblock_content):
  '''Each documentation block starts with a block marker. For example,

  ///Intro: a brief introduction.

  This function returns such a block marker (just the string 'Intro' in
  the example above).'''
  idx = min(item for item in map(doxblock_content.find, (":", "."))
            if item >= 0)

  if idx != None:
    block_type = doxblock_content[:idx].strip()
    block_content = doxblock_content[idx + 1:].strip()
    return (block_type, block_content) 
  else:
    return (None, None)


def get_block_lines(text):
  '''Return the lines in a doxblock independenlty of the actual format of the
  documentation block.
  '''
  if len(text) >= 5 and text.startswith("(**") and text.endswith("*)"):
    return text[3:-2].splitlines()

  elif text.startswith("///"):
    return [line[2:] for line in text[1:].splitlines()]

  return None

known_block_types = \
  {"section": DoxSectionBlock,
   "intro": DoxIntroBlock,
   "example": DoxExampleBlock,
   "same": DoxSameBlock}

def create_blocks_from_classified_slices(classified_slices):
  '''This function creates documentation block objects from the classified text
  slices.
  '''

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
      paragraph = " ".join(lines).strip()
      block_type, content = split_block(paragraph)
      block_type = block_type.lower()
      constructor = known_block_types.get(block_type)
      if constructor != None:
        # Call the constructor for the block
        doxblock = constructor(cs, content)
        doxblocks.append(doxblock)

      else:
        log_msg("Unrecognized: %s\n" % block_type)

  return doxblocks

def add_blocks_to_tree(tree, blocks):
  # Now create the node tree: section, types, procs, instances, ...
  for block in blocks:
    node = block.add_node(tree)
    if node != None:
      node.add_block(block)
      block.set_target(node)

def create_classified_slices_from_text(text):
  dbe = DoxBlockExtractor(text)
  return dbe.extract()

def associate_contexts_to_blocks(blocks, initial_context):
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



