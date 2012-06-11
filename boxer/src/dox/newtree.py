# Copyright (C) 2011, 2012 by Matteo Franchin
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
#   along with Boxer.  If not, see <http://www.gnu.org/licenses/>.

import re
import string

from logger import log_msg


def _normalize_identifier(st):
  '''Normalize a subtype. Example,
  _normalize_identifier("  X  . Y . Zeta") returns "X.Y.Zeta".
  '''
  components = map(str.strip, st.split("."))
  return ".".join(components)


_typedef_re = re.compile(r'\s*([A-Z][A-Za-z0-9_.\s]*)[=]')
_instdef_re = re.compile(r'\s*([a-z][A-Za-z0-9_.\s]*)[=]')
_procdef_re = re.compile(r'\s*([^@]+)\s*[@]\s*([A-Z][A-Za-z0-9_\s.]*)')

  
def dox_classify_code(line):
  '''Parse the line and return a DoxType, a DoxInstance , a DoxProc or None
  depending on whether the line represents a type definition, a procedure
  definition or something that was not understood by the parser.'''
  if line == None:
    return None

  matches_typedef = _typedef_re.match(line)
  if matches_typedef:
    return DoxType(_normalize_identifier(matches_typedef.group(1)))

  matches_procdef = _procdef_re.match(line)
  if matches_procdef:
    left, right = matches_procdef.group(1, 2)
    return DoxProc(left, _normalize_identifier(right))

  matches_instdef = _instdef_re.match(line)
  if matches_instdef:
    return DoxInstance(_normalize_identifier(matches_instdef.group(1)))

  return None


class DoxTreeNode(object):
  node_name = None


class DoxSectionNode(DoxTreeNode):
  node_name = None

  
class DoxLeaf(DoxTreeNode):
  def __init__(self, doxblocks=None, section=None):
    DoxTreeNode.__init__(self)
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
  node_name = 'type'

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

  def is_anonymous(self):
    return not self.name.isalnum()

  def add_child(self, child):
    self.children.append(child)

  def add_parent(self, parent):
    self.parents.append(parent)


class DoxInstance(DoxLeaf):
  node_name = 'instance'

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
    return 'DoxInstance("%s")' % self.name

  def __str__(self):
    return self.name

  def is_anonymous(self):
    return not self.name.isalnum()


class DoxProc(DoxLeaf):
  node_name = 'proc'

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
      if not parent.is_anonymous():
        missing.append(parent)

    if child == None:
      child = DoxType(self.child, section=self.section)
      if not child.is_anonymous():
        missing.append(child)

    parent.add_child(child)
    child.add_parent(parent)
    return missing


class DoxTree(DoxTreeNode):
  def __init__(self):
    DoxTreeNode.__init__(self)
    self.types = {}
    self.procs = {}
    self.instances = {}
    self.all_nodes = {'type': self.types,
                      'proc': self.procs,
                      'instance': self.instances}
    self.sections = []

  def add_node(self, *nodes):
    '''Add a node to the tree.'''
    for node in nodes:
      node_id = str(node)
      selected_nodes = self.all_nodes[node.node_name]
      if node_id in selected_nodes:
        log_msg("Overwriting node '%s' in '%s'" % (node_id, node.node_name))
      selected_nodes[node_id] = node

  def get_type(self, tn, create=True):
    tn = str(tn)
    t = self.types.get(tn, None)
    if t != None:
      return t

    else:
      self.types[tn] = t = DoxType(tn)
      return t

  def create_node_from_source(self, source):
    '''Given a statement of Box source code, generate a corresponding node
    of the tree. The node is added to the tree and returned.'''
    node = dox_classify_code(source)
    if node != None:
      self.add_node(node)
    return node

  def _build_links(self):
    for proc in self.procs.itervalues():
      missing = proc.link_types(self.types)
      if missing:
        names_missing = map(str, missing)

        log_msg("Cannot find %s for procedure %s"
                % (" and ".join(names_missing), proc))

        # Add missing types
        self.add_node(*missing)

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

    self.add_node(*new_types)
    #for t_name in found_subtypes:
    #  self.types.pop(t_name)

  def _process_blocks(self):
    for p in self.procs.itervalues():
      p.process_blocks(self)
    for t in self.types.itervalues():
      t.process_blocks(self)
    for i in self.instances.itervalues():
      i.process_blocks(self)

  def _build_section_list(self):
    self.sections = sections = {}
    for p in self.procs.itervalues():
      sections.setdefault(p.get_section(), []).append(p)
    for t in self.types.itervalues():
      sections.setdefault(t.get_section(), []).append(t)
    for i in self.instances.itervalues():
      sections.setdefault(i.get_section(), []).append(i)

  def process(self):
    self._build_links()
    self._link_subtypes()
    self._process_blocks()
    self._build_section_list()
