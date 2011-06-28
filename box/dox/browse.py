#!/usr/bin/env python

# example basictreeview.py

import pygtk
pygtk.require('2.0')
import gtk

from dox import Dox
from gtktext import DoxTextView, DoxTable, GtkWriter


def _populate_treestore_from_dox(ts, dox):
  tree = dox.tree
  section_names = tree.sections.keys()
  types = tree.types
  type_names = types.keys()
  type_names.sort()

  section_names.sort()
  for section_name in section_names:
    sn = section_name if section_name else "Other stuff..."
    piter = ts.append(None, [sn])

    for type_name in type_names:
      t = types[type_name]
      if t.get_section() == section_name:
        ts.append(piter, [type_name])


class BoxerWindowSettings(object):
  def __init__(self, dox, size=(600, 400), spacing=6,
               title="Box documentation browser"):

    # The documentation object
    self.dox = dox

    # Create the TextView where the documentation will be shown
    self.window_textview = dox_textview = \
      DoxTextView(on_click_link=self._on_click_link)
    self.window_scrolledwin1 = scrolledwin = gtk.ScrolledWindow()
    scrolledwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
    scrolledwin.add(dox_textview)

    self.window_table = dox_table = DoxTable()
    self.window_scrolledwin2 = scrolledwin = gtk.ScrolledWindow()
    scrolledwin.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
    scrolledwin.add(dox_table.treeview)

    dox_textbuffer = dox_textview.get_buffer()
    self.dox_writer = GtkWriter(dox.tree, dox_textbuffer, dox_table)

    # Create a new window
    self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    self.window.set_border_width(spacing)
    self.window.set_title(title)
    self.window.set_size_request(*size)

    self.window.connect("delete_event", self._on_delete_event)

    # The window has one top main region and a bottom region where the
    # ok/cancel buttons are
    self.window_vsplit = gtk.VBox()
    self.window.add(self.window_vsplit)

    # The window is split horizontally into two parts:
    # - on the left we have the zone where we can select the setting to change
    # - on the right we can actually manipulate the setting
    self.window_horsplit = gtk.HPaned()
    self.window_vsplit.pack_start(self.window_horsplit, expand=True,
                                    fill=True, padding=0)

    self.window_button_ok = gtk.Button(label="_Ok")
    self.window_button_cancel = gtk.Button(label="_Cancel")

    self.window_butbox = gtk.HButtonBox()
    self.window_butbox.add(self.window_button_ok)
    self.window_butbox.add(self.window_button_cancel)
    self.window_butbox.set_layout(gtk.BUTTONBOX_END)
    self.window_butbox.set_spacing(spacing)
    self.window_vsplit.pack_start(self.window_butbox, expand=False,
                                  fill=False, padding=0)

    # We first define the right part, which is split vertically in two
    self.window_vsplit2 = gtk.VBox(False, 4)
    self.window_horsplit.pack2(self.window_vsplit2)

    # RIGHT PART: In the upper part we have a text entry, which is initially
    # filled with the current setting and the user can edit in order to change
    # it.

    self.window_vsplit2.pack_start(self.window_scrolledwin1, expand=True,
                                   fill=True, padding=4)
    self.window_vsplit2.pack_start(self.window_scrolledwin2, expand=True,
                                   fill=True, padding=4)

    # create a TreeStore with one string column to use as the model
    self.treestore = treestore = gtk.TreeStore(str)

    _populate_treestore_from_dox(treestore, dox)

    # create the TreeView using treestore
    self.treeview = gtk.TreeView(self.treestore)

    # create the TreeViewColumn to display the data
    self.tvcolumn = gtk.TreeViewColumn('Available settings')

    # add tvcolumn to treeview
    self.treeview.append_column(self.tvcolumn)

    # create a CellRendererText to render the data
    self.cell = gtk.CellRendererText()

    # add the cell to the tvcolumn and allow it to expand
    self.tvcolumn.pack_start(self.cell, True)

    # set the cell "text" attribute to column 0 - retrieve text
    # from that column in treestore
    self.tvcolumn.add_attribute(self.cell, 'text', 0)

    self.treeview.set_search_column(0)       # Make it searchable
    self.tvcolumn.set_sort_column_id(0)      # Allow sorting on the column
    self.treeview.set_reorderable(True)      # Allow drag and drop reordering of rows

    self.treeview.connect("row-activated", self._on_row_activated)
    self.window_button_ok.connect("button-press-event",
                                  self._on_button_ok_press)
    self.window_button_cancel.connect("button-press-event",
                                      self._on_button_cancel_press)

    # Insert objects one inside the other
    self.window_scrolledwin2 = scrolledwin = gtk.ScrolledWindow()
    scrolledwin.set_size_request(int(size[0]/3), -1)
    scrolledwin.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
    scrolledwin.add(self.treeview)
    self.window_horsplit.pack1(scrolledwin)

    self.window.show_all()

    self.option_section = None
    self.option_name = None
    self.option_initial_value = None
    self.option_changed_vals = {}

  def quit(self):
    self.window.hide()
    gtk.main_quit()

  def _on_delete_event(self, widget, event, data=None):
    gtk.main_quit()
    return False

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
  dox.read_recursively("../")
  tree = dox.tree
  tree.process()

  tmp = BoxerWindowSettings(dox)
  main()
