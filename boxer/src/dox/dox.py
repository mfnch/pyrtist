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

import os
import fnmatch

from tree import DoxType, DoxProc, DoxTree
import logger

dox_magic_seq = "///"
dox_comment_seq = "//"
dox_proc_sep = "@"
dox_asgn_sep = "="

(DOXLINETYPE_SKIP,
 DOXLINETYPE_PRE,
 DOXLINETYPE_POST,
 DOXLINETYPE_CONT) = range(4)


def extract_cmnd(s, start=0):
  """Extract a Dox command identifier."""
  n = len(s) + 1
  n1 = s.find(":", start) % n
  n2 = s.find(".", start) % n
  end = min(n1, n2)
  return (s[start:end], s[end + 1:])

def dox_line_type(line):
  sline = line.lstrip()
  if (dox_magic_seq not in line) and (dox_comment_seq not in line):
    return (DOXLINETYPE_SKIP, None, line)

  elif sline.startswith(dox_magic_seq):
    block_type, reminder = extract_cmnd(sline, len(dox_magic_seq))
    return (DOXLINETYPE_PRE, block_type, reminder)

  elif sline.startswith(dox_comment_seq):
    return (DOXLINETYPE_CONT, None, sline[len(dox_comment_seq):])

  else:
    # should check for /// not being contained into a string
    n = len(sline) + 1
    idx = 0
    while idx < n:
      magic_idx = sline.find(dox_magic_seq, idx) % n
      delim_idx = sline.find('"', idx) % n
      if magic_idx < delim_idx:
        block_type, reminder = \
          extract_cmnd(sline, magic_idx + len(dox_magic_seq))
        return (DOXLINETYPE_POST, block_type, reminder)

      elif delim_idx < magic_idx:
        idx = 1 + sline.find('"', delim_idx + 1) % n

      else:
        return (DOXLINETYPE_SKIP, None, line)

def extract_word(s, start=0, wanted=None):
  """Return the portion of 's' starting from position 'start' made of all
  consecutive alpha-numeric characters."""
  wanted = lambda s: s.isalnum() or s in " \t."
  n = len(s)
  idx = n
  for i in range(start, n):
    if not wanted(s[i]):
      idx = i
      break
  return s[start:idx]

def dox_code_type(line):
  if line == None:
    return None

  nmax = len(line) + 1
  n1 = line.find(dox_asgn_sep) % nmax
  n2 = line.find(dox_proc_sep) % nmax

  if nmax == 1 or n1 == n2:
    return None

  elif n1 < n2:
    left = line[:n1].strip()
    return DoxType(left)

  else:
    left = line[:n2].strip()
    right = extract_word(line, n2 + 1).strip()
    return DoxProc(left, right)


class DoxBlocks(object):
  def __init__(self, prev=None):
    self.content = {}
    self.current_content = None
    self.current_name = None
    self.target = None
    self.prev = prev
    self.section = None

  def __str__(self):
    s = "Line '%s' is documented by:\n" % self.target
    for topic in self.content:
      s += "Topic '%s': %s\n" % (topic, self.content[topic])
    return s

  def set_section(self, prev_section):
    return self.content.setdefault("Section", prev_section)

  def new(self, name):
    n = self.current_name
    if n != None:
      self.content[n] = self.current_content
    self.current_name = name.capitalize() if name != None else None
    self.current_content = []

  def append(self, line):
    self.current_content.append(line)

  def finish(self, line):
    self.new(None)
    self.target = line

    # Init from previous, if requested
    if "Same" in self.content:
      if self.prev == None:
        logger.log("Warning: when using `Same' previous was not found.")

      else:
        for prev_name, prev_block in self.prev.content.iteritems():
          self.content.setdefault(prev_name, prev_block)

  def process(self, tree, owner):
    # Select section for documentation block
    section_title = self.content.get("Section", None)
    if section_title != None:
      self.section = " ".join(section_title).strip().capitalize()

    else:
      self.section = None


class DoxFileContentParser(object):
  def __init__(self, content):
    self.current_section = None
    self.lines = content.splitlines()
    self.nr_line = 0
    self.original_line = None # Current line as read by the parse routine
    self.line = None          # Current line after first preprocessing
    self.line_type = None     # Type of line as detected by preprocessing
    self.blocks = None
    self.prev_blocks = None
    self.store = []

  def _parse(self, parse_line, continuing=True):
    if continuing:
      if parse_line():
        return

    while self.nr_line < len(self.lines):
      self.original_line = line = self.lines[self.nr_line]
      self.nr_line += 1
      self.line_type, self.block_type, self.line = dox_line_type(line)
      if parse_line():
        break

  def parse(self):
    self._parse(self._parse_main, continuing=False)
    self.nr_line = None

  def _parse_main(self):
    lt = self.line_type
    if lt == DOXLINETYPE_PRE:
      self.blocks = blocks = DoxBlocks(prev=self.prev_blocks)
      self._parse(self._parse_block)

      if blocks.target not in [None, '']:
        self.store.append(blocks)
        self.prev_blocks = blocks

      self.blocks = None
      self.current_section = blocks.set_section(self.current_section)

    elif lt == DOXLINETYPE_POST:
      blocks = DoxBlocks(prev=self.prev_blocks)
      blocks.new(self.block_type)
      blocks.append(self.line)
      blocks.finish(self.original_line)
      assert blocks.target != None
      self.store.append(blocks)
      self.prev_blocks = blocks
      self.current_section = blocks.set_section(self.current_section)

    return False

  def _parse_block(self):
    line = self.line
    lt = self.line_type
    if lt == DOXLINETYPE_CONT:
      self.blocks.append(line)
      return False

    elif lt == DOXLINETYPE_PRE:
      self.blocks.new(self.block_type)
      self.blocks.append(line)
      return False

    elif lt == DOXLINETYPE_SKIP:
      self.blocks.finish(line)
      return True

    else:
      return True


class DoxFileParser(DoxFileContentParser):
  def __init__(self, filename):
    """Read the documentation content from the given file."""
    self.filename = filename
    f = open(filename, "rt")
    content = f.read()
    f.close()
    DoxFileContentParser.__init__(self, content)

  def read(self):
    self.parse()
    return self.store

class Dox(object):
  def __init__(self):
    self.file = None
    self.tree = DoxTree()

  def log(self, msg, show_hdr=False):
    hdr = ""
    if show_hdr and self.file != None:
      hdr = self.file.filename
      if self.file.nr_line != None:
        hdr += "(%d)" % (self.file.nr_line)
      hdr += ": "
    logger.log(hdr + msg)

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
    self.file = DoxFileParser(filename)
    store = self.file.read()
    if len(store) > 0:
      self.log("File %s:" % filename)
      for doxblocks in store:
        ct = dox_code_type(doxblocks.target)
        if isinstance(ct, DoxType):
          ct.set_owned_blocks(doxblocks)
          self.tree.add_type(ct)

        elif isinstance(ct, DoxProc):
          ct.set_owned_blocks(doxblocks)
          self.tree.add_proc(ct)

        else:
          self.log("Unrecognized documentation block.")
    

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
