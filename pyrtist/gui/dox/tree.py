# Copyright (C) 2011, 2012 by Matteo Franchin
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
  '''Every tree node type is derived from this type.'''
  node_name = None

  def __init__(self, section=None):
    self.section = section
    self.blocks = {}

  def get_section(self):
    '''Get the section this node belongs to.'''
    return self.section

  def set_section(self, section):
    '''Set the section this node belongs to.'''
    if self.section != None and section != self.section:
      log_msg("Contraddictory section assignment for node '%s': "
              "got '%s', was '%s'" % (self, section, self.section))
    self.section = section

  def add_block(self, block):
    '''Add a documentation block to this node.
    For example, if a source file contains the lines:

      (**Intro: some brief description of MyType. *)
      MyType = (Int a, b)

    Then this function will be called to add the ``Intro'' block to the
    ``MyType'' node.
    '''
    block_name = block.block_name.lower()
    if block.multiple_allowed:
      self.blocks.setdefault(block_name, []).append(block)
    else:
      self.blocks[block_name] = block

  def get_block(self, block_name):
    '''Retrieve the blocks associated to the node with the given name.
    If the block type admit multiple association, then return a list,
    otherwise - if the block can be associated only to one tree node at a time
    - return the object.
    '''
    return self.blocks.get(block_name.lower(), None)

  def get_blocks(self):
    '''Get all the documentation blocks associated to this node.'''
    return self.blocks.values()


class DoxParentNode(DoxTreeNode):
  '''A node that can contain other nodes.'''

  def __init__(self, *args, **kwargs):
    DoxTreeNode.__init__(self, *args, **kwargs)
    self.types = {}
    self.procs = {}
    self.instances = {}
    self.all_nodes = {'type': self.types,
                      'proc': self.procs,
                      'instance': self.instances}

  def add_node(self, *nodes):
    '''Add a node to the tree.'''
    for node in nodes:
      node_id = str(node)
      selected_nodes = self.all_nodes[node.node_name]
      if node_id in selected_nodes:
        log_msg("Overwriting node '%s' in '%s'" % (node_id, node.node_name))
      selected_nodes[node_id] = node

  def get_node(self, nodetype, nodename, create=True):
    '''Return the node of type ``nodetype'' and name ``nodename''. If the node
    is not present then: None is returned if ``create'' is false, or the node
    is created and returned if ``create'' is true. Default for ``create'' is
    True.

    Example: parentnode.get_node(DoxType, 'mybeautifulnode', create=False)
    '''
    group = self.all_nodes.get(nodetype.node_name)
    node = group.get(nodename, None)
    if create:
      if node != None:
        return node
      else:
        group[nodename] = newnode = nodetype(nodename)
        return newnode

    else:
      return node

  def get_nodes(self, nodetype):
    '''Get all the nodes of the specified type.

    Example: DoxParentNode.get_nodes(DoxType) to get all the type nodes.'''
    return self.all_nodes.get(nodetype.node_name).values()

  def get_type(self, name, *args, **kwargs):
    '''DoxParentNode.get_type(name, **kwargs) is equivalent to
    DoxParentNode.get_node(DoxType, name, **kwargs).'''
    return self.get_node(DoxType, name, *args, **kwargs)


class DoxSectionNode(DoxParentNode):
  '''Section node (corresponding to a section block).'''
  node_name = 'section'

  def __init__(self, name, *args, **kwargs):
    DoxParentNode.__init__(self, *args, **kwargs)
    self.name = name
    self.subsections = {}
    self.nodes = {}

  def __str__(self):
    return self.name

  def get_subsections(self):
    '''Get all the subsections of this section.'''
    return self.subsections.values()

  def get_subsection(self, section_path, create=False):
    '''Get a subsection corresponding to the given path.'''
    sections = section_path.split('.', 1)
    if len(sections) == 1:
      section_name = sections[0]
      assert len(section_name) > 0, 'Empty section name'
      subsection = self.subsections.get(section_name)
      if subsection != None:
        return subsection

      elif create:
        return self.subsections.setdefault(section_name,
                                           DoxSectionNode(section_name))

      else:
        raise ValueError("Subsection '%s' of '%s' was not found"
                         % (section_name, self.name))

    else:
      section_name, children = sections
      subsection = self.get_subsection(section_name, create=create)
      return subsection.get_subsection(children, create=create)


class DoxType(DoxTreeNode):
  '''Node corresponding to a type definition.'''

  node_name = 'type'

  def __init__(self, name, *args, **kwargs):
    DoxTreeNode.__init__(self, *args, **kwargs)

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


class DoxInstance(DoxTreeNode):
  node_name = 'instance'

  def __init__(self, name, *args, **kwargs):
    DoxTreeNode.__init__(self, *args, **kwargs)

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


class DoxProc(DoxTreeNode):
  node_name = 'proc'

  def __init__(self, child, parent, *args, **kwargs):
    DoxTreeNode.__init__(self, *args, **kwargs)
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


class DoxTree(DoxParentNode):
  def __init__(self):
    DoxParentNode.__init__(self)
    self.sections = []
    self.root_section = DoxSectionNode('root')

  def get_root_section(self):
    '''Get the root section in the tree. The root section is a DoxSectionNode
    object which is used to organise the content of the tree.'''
    return self.root_section

  def create_node_from_source(self, source):
    '''Given a statement of Box source code, generate a corresponding node
    of the tree. The node is added to the tree and returned.'''
    node = dox_classify_code(source)
    if node != None:
      existing_node = self.get_node(node, str(node), create=False)
      if existing_node:
        return existing_node

      else:
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
    self._build_section_list()
