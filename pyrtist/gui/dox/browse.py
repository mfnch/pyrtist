#!/usr/bin/env python
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

import pygtk
pygtk.require('2.0')
import gtk
import gobject

from ..config import get_configuration
from ..editable import BoxViewArea

from dox import Dox
from tree import DoxTree, DoxType, DoxInstance, DoxSectionNode
from gtktext import DoxTextView, DoxTable, GtkWriter
from writer import Writer


class DoxBrowser(object):
  def __init__(self, dox, size=(640, 560), spacing=6,
               title="Box documentation browser",
               quit_gtk=False):
    self.size = size
    self.dox = dox
    tree = dox.tree
    self.option_section = None
    self.option_name = None
    self.option_initial_value = None
    self.option_changed_vals = {}
    self.quit_gtk = quit_gtk

    # Create the window
    self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    self.window.set_border_width(spacing)
    self.window.set_title(title)
    self.window.set_size_request(*size)
    self.window.connect("delete_event", self._on_delete_event)

    # RIGHT/TOP PART OF THE WINDOW
    #
    # This is composed by a descriptor window at the top (showing the text
    # describing the selected object), plus a list of properties of the object
    # at the bottom
    right_top = gtk.VBox(False, 4)
    description_text = self._build_widget_description()
    properties_list = self._build_widget_combinations()
    right_top.pack_start(description_text, expand=True, fill=True, padding=4)
    right_top.pack_start(properties_list, expand=True, fill=True, padding=4)

    # Create a GtkWriter which is the target of our documentation system.
    # The extracted documentation will be formatted and redirected to the
    # two views we defined above
    dox_textbuffer = self.window_textview.get_buffer()
    self.dox_writer = GtkWriter(tree, dox_textbuffer, self.window_table)

    # LEFT/TOP PART OF THE WINDOW
    #
    # The left part of the window is shared between the treeview and the
    # preview area (they are the two children of a VPaned). The preview area is
    # initially hidden and the treeview occupies the whole left part of the
    # window. The idea is that initially the treeview represents the most
    # important area for the user, as he/she is probably trying to find/locate
    # the documentation for a given object.  Once the object has been found the
    # user can select it. If the objecy has a preview section, then the
    # corresponding preview image should be shown. Whent the preview image is
    # shown, the space allocated to the treeview is drastically reduced. This
    # should be fine, as - at this stage - the treeview is less important (the
    # user did find the object and is now interested in reading the
    # corresponding documentation).

    # The documentation finder/selector (a scrollable treeview)
    selector = self._build_widget_selector()

    self.previewer = previewer = self._build_widget_preview()
    self.set_preview_code(None, refresh=False)
    left_top = gtk.VPaned()
    left_top.pack1(selector, resize=True, shrink=True)
    left_top.pack2(previewer, resize=False, shrink=True)

    # TOP PART OF THE WINDOW: LEFT/TOP + RIGHT/TOP
    #
    # The window is split horizontally into two parts:
    # - on the left we have the zone where we can browse the available
    #   documentation (in a treeview object) and we can (eventually) see a
    #   preview of the selected object.
    # - on the right we can read the documentation of the selected object
    #   and inspect its various properties.
    top_part = gtk.HPaned()
    top_part.pack1(left_top)
    top_part.pack2(right_top)

    # BOTTOM PART OF THE WINDOW
    #
    # Create the buttons which will be positioned at the bottom of the window
    self.window_button_hide = gtk.Button(label="_Hide")
    self.window_butbox = bottom_part = gtk.HButtonBox()
    bottom_part.add(self.window_button_hide)
    bottom_part.set_layout(gtk.BUTTONBOX_END)
    bottom_part.set_spacing(spacing)

    # The window has one top main region and a bottom region where the
    # ok/cancel buttons are
    window_content = gtk.VBox()
    window_content.pack_start(top_part, expand=True, fill=True, padding=0)
    window_content.pack_start(bottom_part, expand=False, fill=False, padding=0)
    self.window.add(window_content)

    self.window_button_hide.connect("button-press-event",
                                      self._on_button_hide_press)

  def _build_widget_description(self):
    '''Internal: called during initialization to build the widget which shows
    the documentation text for the currently selected item.'''
    # Create the TextView where the documentation will be shown
    self.window_textview = dox_textview = \
      DoxTextView(on_click_link=self._on_click_link)
    scrolledwin1 = gtk.ScrolledWindow()
    scrolledwin1.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
    scrolledwin1.set_shadow_type(gtk.SHADOW_IN)
    scrolledwin1.add(dox_textview)
    return scrolledwin1

  def _build_widget_combinations(self):
    '''Internal: called during initialization to build the widget containing
    the list of type combinations.'''

    # Create the table where the procedure of the current type are shown
    self.window_table = dox_table = \
      DoxTable(on_click_link=self._on_click_link)
    scrolledwin = gtk.ScrolledWindow()
    scrolledwin.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
    scrolledwin.set_shadow_type(gtk.SHADOW_IN)
    viewport = gtk.Viewport()
    viewport.add(dox_table)
    viewport.set_shadow_type(gtk.SHADOW_NONE)
    scrolledwin.add(viewport)
    return scrolledwin

  def _build_widget_selector(self):
    '''Internal: called during initialization to build the treeview which
    allow to find and browse the documentation.'''

    # create and populate the treestore
    self.treestore = treestore = \
      gtk.TreeStore(gobject.TYPE_STRING,  # Type name
                    gobject.TYPE_STRING,  # Brief description
                    gobject.TYPE_BOOLEAN) # Visible?
    self._populate_treestore_from_dox()

    # the treestore filter (to show and hide rows as a result of a search)
    self.treestore_flt = treestore_flt = treestore.filter_new()
    treestore_flt.set_visible_column(2)

    # create the TreeView using the treestore
    tv = gtk.TreeView(treestore_flt)
    tvcols = []
    for nr_col, tvcol_name in enumerate(("Type", "Description")):
      tvcol = gtk.TreeViewColumn(tvcol_name)
      tvcol.set_resizable(True)
      tvcols.append(tvcol)
      tv.append_column(tvcol)
      cell = gtk.CellRendererText()
      tvcol.pack_start(cell, True)
      tvcol.add_attribute(cell, 'text', nr_col)

    tvcols[0].set_sort_column_id(0)
    tv.set_search_column(0)
    self.treeview = tv

    # Put the treeview inside a scrolled window
    scrolledwin = gtk.ScrolledWindow()
    scrolledwin.set_size_request(int(self.size[0]/2), -1)
    scrolledwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
    scrolledwin.add(tv)

    # We put a text entry and a button at the top of the treeview. This can
    # be used to search/filter items in the treeview
    label = gtk.Label("Search:")
    self.entry = entry = gtk.Entry()
    hsplit = gtk.HBox(False, 4)
    hsplit.pack_start(label, expand=False, fill=True, padding=4)
    hsplit.pack_start(entry, expand=True, fill=True, padding=4)

    vsplit = gtk.VBox(False, 4)
    vsplit.pack_start(hsplit, expand=False, fill=True, padding=4)
    vsplit.pack_start(scrolledwin, expand=True, fill=True, padding=4)

    # Connect events:
    # ---
    # Connect the row-activate event, so that we can show the documentation
    # when an item is selected in the treeview
    tv.connect("row-activated", self._on_row_activated)

    # Make sure that we can filter the entries of the treeview, by typing some
    # keywords on the entry and pressing [RETURN]
    entry.connect("activate", self._on_entry_search_activated)

    return vsplit

  def _build_widget_preview(self):
    '''Internal: called during initialization to build the preview window.'''
    cfg = get_configuration()
    cfg_dict = {"box_executable": cfg.get("Box", "exec"),
                "box_include_dirs": cfg.get("Library", "dir"),
                "refpoint_size": cfg.getint("GUIView", "refpoint_size")}
    self.viewarea = viewarea = BoxViewArea(config=cfg_dict)

    scroll_win = gtk.ScrolledWindow()
    scroll_win.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
    scroll_win.add(viewarea)
    scroll_win.set_size_request(200, 200)
    return scroll_win

  def set_preview_code(self, src, refresh=True, update_visibility=True):
    '''Set the Box source which is used to render the preview window.'''

    code = ('(**expand:boxer-boot*)\n(**boxer-version:0,2,0*)\n'
            '(**end:expand*)\n')
    if src:
      code += src

    else:
      code += ('fg = Figure[BBox[v = (15, 10), v, -v], '
               'Text[Font["helvetica-bold", 5, Color[0.6]], (0, 0), '
               '"  PREVIEW\\n      NOT\\nAVAILABLE"]], (**view:fg*)')

    if src != None:
      document = self.viewarea.get_document()
      document.new()
      document.load_from_str(code)

    if refresh:
      #self.viewarea.reset()
      self.viewarea.zoom_off()
      #self.viewarea.refresh()

    if update_visibility:
      if src == None:
        self.previewer.hide()
      else:
        self.previewer.show()

  def show(self, section=None, topic=None):
    """Show the window."""
    self.window.show_all()
    self.set_preview_code(None)

    if topic != None:
      self._show_doc(section, topic)
      self.entry.set_text(topic)
      self.entry.grab_focus()

  def hide(self):
    """Hide the window."""
    self.window.hide()

  def _populate_treestore_from_dox(self, **kwargs):
    root_section = self.dox.tree.get_root_section()
    self._populate_section_node(None, root_section, **kwargs)

  def _populate_section_node(self, parent, section, **kwargs):
    # Get the subsections of this section
    sections = section.get_subsections()
    types = section.get_nodes(DoxType)
    instances = section.get_nodes(DoxInstance)

    # Used to sort the items
    cmpstr = lambda x, y: cmp(str(x), str(y))

    for items in (sections, types, instances):
      items.sort(cmpstr)
      for item in items:
        if not isinstance(item, DoxType) or item.subtype_parent == None:
          self._add_any_node(parent, item, **kwargs)

  def _populate_type_node(self, parent, type_node, **kwargs):
    # Add subtypes of the given type to the type node
    for subtype_node in type_node.subtype_children:
      self._add_any_node(parent, subtype_node, **kwargs)

  def _add_any_node(self, parent, node, **kwargs):
    # Add a node to the treestore object
    visible = kwargs.get('visible', True)
    flt = kwargs.get('flt')
    description = self.dox_writer.gen_brief_intro(node)
    if flt == None or flt(node, description):
      child = self.treestore.append(parent, [str(node), description, visible])

      if isinstance(node, DoxType):
        self._populate_type_node(child, node, **kwargs)

      elif isinstance(node, DoxSectionNode):
        self._populate_section_node(child, node, **kwargs)

  def _add_entry_to_treestore(self, piter, t, flt=None, visible=True):
    ts = self.treestore
    description = self.dox_writer.gen_brief_intro(t)
    if flt == None or flt(t, description):
      new_piter = ts.append(piter, [t.name, description, visible])

  def quit(self):
    self.window.hide()
    if self.quit_gtk:
      gtk.main_quit()

  def _on_delete_event(self, widget, event, data=None):
    self.quit()
    return True

  def _on_button_hide_press(self, *args):
    self.quit()

  def _on_row_activated(self, treeview, path, view_column):
    ts = self.treestore
    flt_ti = treeview.get_selection().get_selected()[1]
    ti = self.treestore_flt.convert_iter_to_child_iter(flt_ti)

    parent_iter = ts.iter_parent(ti)
    if parent_iter:
      section = ts[parent_iter][0]
      option = ts[ti][0]
      self._show_doc(section, option)

  def _on_click_link(self, widget, link, event):
    self._show_doc(None, link)

  def _filter_type_in_treeview(self, treemodelrow, flt):
    type_name = treemodelrow[0]
    brief_description = treemodelrow[1]

    # Find whether this entry matches the filter
    self_visible = flt(type_name, brief_description)
    children_visible = False

    # Find whether some of the children match the filter
    children = treemodelrow.iterchildren()
    if children:
      for child in children:
        children_visible |= self._filter_type_in_treeview(child, flt)

    # Show the entry if it or one of its children are visible
    visible = self_visible or children_visible
    treemodelrow[-1] = visible
    return visible

  def get_brief_desc(self, search_str):
    """Given a search item, return a brief description.
    Return None if nothing can be associated to the search item."""
    t = self.dox.tree.types.get(search_str, None)
    if t:
      return self.dox_writer.gen_brief_intro(t)
    else:
      return None

  def filter_treeview(self, flt):
    types = self.dox.tree.types
    ts = self.treestore
    for section in ts:
      visible = False
      types_of_section = section.iterchildren()
      if types_of_section:
        for t in types_of_section:
          visible |= self._filter_type_in_treeview(t, flt)
      section[-1] = visible

  def expand_visible_rows(self, treemodelrow=None):
    if treemodelrow == None:
      for section in self.treestore:
        assert section != None
        self.expand_visible_rows(section)
      return

    visible = treemodelrow[-1]
    children = treemodelrow.iterchildren()
    if visible and children:
      # First, expand itself
      path = self.treestore_flt.convert_child_path_to_path(treemodelrow.path)
      self.treeview.expand_row(path, False)
 
      # Then expand the children
      for child in children:
        visible = child[-1]
        if visible:
          self.expand_visible_rows(child)

  def _on_entry_search_activated(self, widget):
    search_str = widget.get_text()
    def flt(*args):
      for arg in args:
        if search_str.lower() in arg.lower():
          return True
      return False

    self.filter_treeview(flt)

    if search_str != "":
      self.expand_visible_rows()
    
  def _show_doc(self, section, search_str):
    tree = self.dox.tree
    name = (search_str if "@" not in search_str
            else search_str.split("@", 2)[1])

    output = None
    t = tree.types.get(name)
    if t != None:
      output = self.dox_writer.gen_type_section(t, None)

    else:
      t = tree.instances.get(name)
      if t != None:
        output = self.dox_writer.gen_instance_section(t)

    if output:
      self.window_textview.get_buffer().set_text('')
      self.window_textview.set_content(output)

      # Set the preview code for the preview window
      preview_code = None
      preview_block = t.get_block('Preview')
      if preview_block:
        preview_code = preview_block.get_code()

      self.set_preview_code(preview_code)
      

def main():
  gtk.main()

if __name__ == "__main__":

  dox = Dox()
  dox.read_recursively("../../../box")
  tree = dox.tree
  tree.process()

  tmp = DoxBrowser(dox, quit_gtk=True)
  tmp.show()
  main()
