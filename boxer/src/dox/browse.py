#!/usr/bin/env python

# example basictreeview.py

import pygtk
pygtk.require('2.0')
import gtk

from dox import Dox
from gtktext import DoxTextView, DoxTable, GtkWriter


class DoxBrowser(object):
  def __init__(self, dox, size=(760, 560), spacing=6,
               title="Box documentation browser",
               quit_gtk=False):
    self.dox = dox
    tree = dox.tree
    self.option_section = None
    self.option_name = None
    self.option_initial_value = None
    self.option_changed_vals = {}
    self.quit_gtk = quit_gtk

    # Create the TextView where the documentation will be shown
    self.window_textview = dox_textview = \
      DoxTextView(on_click_link=self._on_click_link)
    scrolledwin1 = gtk.ScrolledWindow()
    scrolledwin1.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
    scrolledwin1.add(dox_textview)

    # Create the table were the procedure of the current type are shown
    self.window_table = dox_table = \
      DoxTable(on_click_link=self._on_click_link)
    scrolledwin2 = gtk.ScrolledWindow()
    scrolledwin2.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
    scrolledwin2.add_with_viewport(dox_table)

    # Put one at the top, the other at the bottom
    vsplit3 = gtk.VBox(False, 4)
    vsplit3.pack_start(scrolledwin1, expand=True, fill=True, padding=4)
    vsplit3.pack_start(scrolledwin2, expand=True, fill=True, padding=4)

    dox_textbuffer = dox_textview.get_buffer()
    self.dox_writer = GtkWriter(tree, dox_textbuffer, dox_table)

    # Create a new window
    self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    self.window.set_border_width(spacing)
    self.window.set_title(title)
    self.window.set_size_request(*size)

    self.window.connect("delete_event", self._on_delete_event)
    
    self.window_button_ok = gtk.Button(label="_Ok")
    self.window_button_cancel = gtk.Button(label="_Cancel")

    self.window_butbox = butbox = gtk.HButtonBox()
    butbox.add(self.window_button_ok)
    butbox.add(self.window_button_cancel)
    butbox.set_layout(gtk.BUTTONBOX_END)
    butbox.set_spacing(spacing)

    # create and populate the treestore
    self.treestore = treestore = gtk.TreeStore(str, str)
    self._populate_treestore_from_dox(tree.types)

    # create the TreeView using the treestore
    self.treeview = tv = gtk.TreeView(self.treestore)
    tvcols = []
    for nr_col, tvcol_name in enumerate(("Type", "Description")):
      tvcol = gtk.TreeViewColumn(tvcol_name)
      tvcols.append(tvcol)
      tv.append_column(tvcol)
      cell = gtk.CellRendererText()
      tvcol.pack_start(cell, True)
      tvcol.add_attribute(cell, 'text', nr_col)

    tvcols[0].set_sort_column_id(0)
    tv.set_search_column(0)

    # Insert objects one inside the other
    scrolledwin = gtk.ScrolledWindow()
    scrolledwin.set_size_request(int(size[0]/2), -1)
    scrolledwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
    scrolledwin.add(tv)

    label = gtk.Label("Search:")
    entry = gtk.Entry()
    hsplit1 = gtk.HBox(False, 4)
    hsplit1.pack_start(label, expand=False, fill=True, padding=4)
    hsplit1.pack_start(entry, expand=True, fill=True, padding=4)

    vsplit2 = gtk.VBox(False, 4)
    vsplit2.pack_start(hsplit1, expand=False, fill=True, padding=4)
    vsplit2.pack_start(scrolledwin, expand=True, fill=True, padding=4)

    # The window is split horizontally into two parts:
    # - on the left we have the zone where we can select the setting to change
    # - on the right we can actually manipulate the setting
    horsplit = gtk.HPaned()
    horsplit.pack1(vsplit2)
    horsplit.pack2(vsplit3)

    # The window has one top main region and a bottom region where the
    # ok/cancel buttons are
    vsplit1 = gtk.VBox()
    vsplit1.pack_start(horsplit, expand=True, fill=True, padding=0)
    vsplit1.pack_start(butbox, expand=False, fill=False, padding=0)
    self.window.add(vsplit1)

    tv.connect("row-activated", self._on_row_activated)
    self.window_button_ok.connect("button-press-event",
                                  self._on_button_ok_press)
    self.window_button_cancel.connect("button-press-event",
                                      self._on_button_cancel_press)

  def show(self):
    """Show the window."""
    self.window.show_all()

  def hide(self):
    """Hide the window."""
    self.window.hide()

  def _populate_treestore_from_dox(self, types):
    ts = self.treestore
    tree = self.dox.tree
    section_names = tree.sections.keys()
    type_names = types.keys()
    type_names.sort()

    section_names.sort()
    for section_name in section_names:
      sn = (section_name if section_name else "Other stuff...")
      piter = ts.append(None, [sn, "Section description"])

      for type_name in type_names:
        t = types[type_name]
        if t.get_section() == section_name:
          if t.subtype_parent == None:
            self._add_entry_to_treestore(piter, t)

  def _add_entry_to_treestore(self, piter, t):
    ts = self.treestore
    description = self.dox_writer.gen_brief_intro(t)
    new_piter = ts.append(piter, [t.name, description])
    for st in t.subtype_children:
      self._add_entry_to_treestore(new_piter, st)

  def quit(self):
    self.window.hide()
    if self.quit_gtk:
      gtk.main_quit()

  def _on_delete_event(self, widget, event, data=None):
    self.quit()
    return True

  def _on_button_ok_press(self, *args):
    self.quit()

  def _on_button_cancel_press(self, *args):
    self.quit()

  def _on_row_activated(self, treeview, path, view_column):
    selection = treeview.get_selection()
    ts, ti = selection.get_selected()
    parent_iter = ts.iter_parent(ti)
    if parent_iter:
      section = ts.get_value(parent_iter, 0)
      option = ts.get_value(ti, 0)
      self._show_doc(section, option)

  def _on_click_link(self, widget, link, event):
    self._show_doc(None, link)
    
  def _show_doc(self, section, type_name):
    tree = self.dox.tree
    types = tree.types
    t = types.get(type_name)
    if t != None:
      self.window_textview.get_buffer().set_text("")
      writer = self.dox_writer
      output = writer.gen_type_section(t, None)
      self.window_textview.set_content(output)


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
