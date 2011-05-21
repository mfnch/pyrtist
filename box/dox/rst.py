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

from tree import DoxItem, DoxType, DoxProc, DoxTree
from writer import Writer

def table_to_string(tab):
  # Find the lengths for the columns
  col_lengths = map(lambda *args: 2 + max(map(len, args)), *tab)

  # String used to separater rows
  middle = "+".join(map(lambda i: "-"*i, col_lengths))
  first_row = "+" + middle + "+\n"
  sep_row = "+" + middle + "+\n"

  return \
    (first_row +
     sep_row.join("| " + "| ".join(entry.ljust(col_lengths[i] - 1)
                                   for i, entry in enumerate(row)) + "|\n"
                  for row in tab) +
     first_row)

def get_item_intro(item):
  if item != None:
    db = item.doxblocks
    if db != None and "Intro" in db.content:
      return " ".join(db.content["Intro"]).strip().capitalize()
  return None

def rst_normalize(s):
  """Normalize the string similarly to what RST does (so that reference names
  are whitespace-neutral and case-insensitive)."""
  return s.lower()


class RSTWriter(Writer):
  def __init__(self, tree, docinfo={}, nest_subtypes=True):
    Writer.__init__(self, tree, docinfo=docinfo)
    self.out = ""
    self.nest_subtypes = nest_subtypes
    self.type_name_map = {}
    self._find_duplicates()

  def _find_duplicates(self):
    # Find duplicate types (with the same name but different case), so that
    # we can treat them alternatively (is there a way to convince Rest to be
    # case sensitive???
    encountered_types = {}
    duplicate_types = {}
    ts = self.tree.types
    for t_name, t in ts.iteritems():
      normalized_name = rst_normalize(t_name)
      dups = encountered_types.setdefault(normalized_name, [])
      dups.append(t)

      if len(dups) == 2:
        duplicate_types[normalized_name] = dups

    type_name_map = self.type_name_map
    for t_name, dups in duplicate_types.iteritems():
      for i, t in enumerate(dups):
        unique_name = t_name + str(i)
        type_name_map[str(t)] = unique_name

  def gen_type_link(self, t):
    t_name = str(t)
    unique_name = self.type_name_map.get(t_name, None)
    return ("`%s`_" % t_name if unique_name == None
            else "|%s|_" % unique_name)

  def gen_section_title(self, title, level=0):
    char = "=-~.`"[min(level, 4)]
    return "%s\n%s\n\n" % (title, char*len(title))

  def gen_type_section_title(self, t, level=0):
    t_name = str(t)
    unique_name = self.type_name_map.get(t_name, None)
    s = ""
    if unique_name != None:
      s = (  ".. |%s| replace:: %s\n" % (unique_name, t_name)
           + ".. _%s:\n\n" % unique_name)
    return s + self.gen_section_title(t_name, level)

  def gen_target_section(self, target, section="Intro"):
    pieces = Writer.gen_target_section(self, target, section)
    if pieces != None:
      text = "".join(pieces).strip()
      text = text[0].upper() + text[1:] if len(text) > 0 else text
      return text

    else:
      return None

  def write(self, s):
    self.out += s

  def writeln(self, s=""):
    self.out += s + "\n"

  def gen_type_section(self, t, level=0):
    self.write(self.gen_type_section_title(t, level=level))
    intro = self.gen_target_section(t)
    if intro != None:
      self.writeln(intro)
      self.writeln()

    children = map(str, t.children)
    if len(children) > 0:
      children.sort()
      links = map(self.gen_type_link, children)
      s = "**Uses:** " + ", ".join(links)
      self.writeln(s)
      self.writeln()

    proc_list = []
    for child in t.children:
      proc = DoxProc(child, t)
      intro = self.gen_target_section(self.tree.procs.get(str(proc), None))
      if intro != None:
        proc_list.append((self.gen_type_link(child), intro))

    if len(proc_list) > 0:
      self.writeln(table_to_string(proc_list))
      self.writeln()

    parents = map(str, t.parents)
    if len(parents) > 0:
      parents.sort()
      links = map(self.gen_type_link, parents)
      s = "**Used by:** " + ", ".join(links)
      self.writeln(s)
      self.writeln()

    subtype_children = map(str, t.subtype_children)
    if len(subtype_children) > 0:
      subtype_children.sort()
      links = map(self.gen_type_link, subtype_children)
      s = "**Subtypes:** " + ", ".join(links)
      self.writeln(s)
      self.writeln()

    if self.nest_subtypes:
      for st in t.subtype_children:
        self.gen_type_section(st, level=level+1)

  def gen_index(self, level=0):
    title = self.docinfo.get("index_title", "Documentation index")
    return (  ".. contents:: %s\n" % title
            + "   :depth: 3\n\n")

  def gen(self, level=0):
    self.out = ""

    title = self.docinfo.get("title", None)
    if title != None:
      self.write(self.gen_section_title(title))
      level += 1

    if self.docinfo.get("has_index", True):
      self.write(self.gen_index(level))

    types = self.tree.types
    typenames = types.keys()
    typenames.sort()
    for typename in typenames:
      t = types[typename]

      if t.subtype_parent == None or self.nest_subtypes == False:
        self.gen_type_section(t, level)

    return self.out
