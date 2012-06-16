# Copyright (C) 2011, 2012 by Matteo Franchin
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

import os
import re
import fnmatch

from logger import log_msg
from tree import DoxType, DoxProc, DoxInstance, DoxTree
from context import Context
import analyzer


class Dox(object):
  def __init__(self):
    self.file = None
    self.tree = DoxTree()
    self.context = Context()

  def log(self, msg, show_hdr=False):
    hdr = ""
    if show_hdr and self.file != None:
      hdr = self.file.filename
      if self.file.nr_line != None:
        hdr += "(%d)" % (self.file.nr_line)
      hdr += ": "
    log_msg(hdr + msg)

  def read_recursively(self, rootpath,
                       extensions = ["*.dox", "*.bxh", "*.box"]):
    for dirpath, dirnames, filenames in os.walk(rootpath):
      doxfiles = []
      for extension in extensions:
        doxfiles.extend(fnmatch.filter(filenames, extension))

      if doxfiles:
        doxfiles = list(set(doxfiles)) # Remove duplicates
        for filename in doxfiles:
          self.read_file(os.path.join(dirpath, filename))

  def read_file(self, filename):
    """Read documentation content from the given file."""

    with open(filename, "r") as f:
      text = f.read()

    slices = analyzer.create_classified_slices_from_text(text)
    blocks = analyzer.create_blocks_from_classified_slices(slices)
    context = self.context.create_context(sourcefile=filename,
                                          section=None)
    self.context = analyzer.associate_contexts_to_blocks(blocks, context)
    analyzer.add_blocks_to_tree(self.tree, blocks)


if __name__ == "__main__":
  import sys
  dox = Dox()
  dox.read_recursively(sys.argv[1])
  tree = dox.tree
  tree.process()

  from rst import RSTWriter
  docinfo = \
    {"title": "Box Reference Manual",
     "has_index": True,
     "index_title": "Index of available object types",
     "out_file": "out"}

  writer = RSTWriter(dox.tree, docinfo=docinfo)
  writer.save()
