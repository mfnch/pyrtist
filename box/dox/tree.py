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

import string


class DoxItem(object):
  def __init__(self):
    pass


class DoxLeaf(DoxItem):
  def __init__(self, doxblocks=None, section=None):
    DoxItem.__init__(self)
    self.doxblocks = doxblocks
    self.section = section

  def set_owned_blocks(self, doxblocks):
    self.doxblocks = doxblocks

  def process_blocks(self, tree):
    if self.doxblocks != None:
      self.doxblocks.process(tree, self)

  def get_section(self):
    return self.doxblocks.section if self.doxblocks != None else None


class DoxType(DoxLeaf):
  def __init__(self, name, doxblocks=None, section=None):
    DoxLeaf.__init__(self, doxblocks, section=section)

    self.name = name
    self.children = []
    self.parents = []
    self.subtype_children = []

    parts = name.rsplit(".", 1)
    subtype_parent, subtype = parts if len(parts) > 1 else (None, None)
    self.subtype_parent = subtype_parent
    self.subtype = subtype

  def __repr__(self):
    return 'DoxType("%s")' % self.name

  def __str__(self):
    return self.name

  def add_child(self, child):
    self.children.append(child)

  def add_parent(self, parent):
    self.parents.append(parent)


class DoxProc(DoxLeaf):
  def __init__(self, child, parent, doxblocks=None, section=None):
    DoxLeaf.__init__(self, doxblocks, section=section)
    self.child = child
    self.parent = parent

  def __repr__(self):
    return 'DoxProc("%s", "%s")' % (self.child, self.parent)

  def __str__(self):
    return "%s@%s" % (self.child, self.parent)

  def link_types(self, types):
    parent = types.get(self.parent, None)
    child = types.get(self.child, None)

    # If the types are missing, create them and put them in the same section.
    missing = []
    if parent == None:
      parent = DoxType(self.parent, section=self.section)
      missing.append(parent)

    if child == None:
      child = DoxType(self.child, section=self.section)
      missing.append(child)

    parent.add_child(child)
    child.add_parent(parent)
    return missing


class DoxTree(DoxItem):
  def __init__(self):
    DoxItem.__init__(self)
    self.types = {}
    self.procs = {}
    self.sections = []

  def log(self, msg):
    print msg

  def add_type(self, *ts):
    for t in ts:
      self.types[str(t)] = t

  def add_proc(self, *ps):
    for p in ps:
      self.procs[str(p)] = p

  def get_type(self, tn, create=True):
    tn = str(tn)
    t = self.types.get(tn, None)
    if t != None:
      return t

    else:
      print "xxxxxxxxxxxxx Creating new type %s" % str(tn)
      self.types[tn] = t = DoxType(tn)
      return t

  def _build_links(self):
    for proc in self.procs.itervalues():
      missing = proc.link_types(self.types)
      if missing:
        names_missing = map(str, missing)

        self.log("Cannot find %s for procedure %s"
                 % (" and ".join(names_missing), proc))

        # Add missing types
        self.add_type(*missing)

  def _link_subtypes(self):
    new_types = []
    found_subtypes = []

    for t_name, t in self.types.iteritems():
      if t.subtype_parent != None:
        found_subtypes.append(t_name)
        stp_name = t.subtype_parent
        stp = self.types.get(stp_name, None)
        if stp == None:
          stp = DoxType(stp_name)
          new_types.append(stp)
        stp.subtype_children.append(t)

    self.add_type(*new_types)
    #for t_name in found_subtypes:
    #  self.types.pop(t_name)

  def _process_blocks(self):
    for p in self.procs.itervalues():
      p.process_blocks(self)
    for t in self.types.itervalues():
      t.process_blocks(self)

  def _build_section_list(self):
    self.sections = sections = {}
    for p in self.procs.itervalues():
      sections.setdefault(p.get_section(), []).append(p)
    for t in self.types.itervalues():
      sections.setdefault(t.get_section(), []).append(t)

  def process(self):
    self._build_links()
    self._link_subtypes()
    self._process_blocks()
    self._build_section_list()
