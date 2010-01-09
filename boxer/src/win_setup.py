#!/usr/bin/env python

# example basictreeview.py

import pygtk
pygtk.require('2.0')
import gtk
from boxer import debug

spacing = 6

class ConfigTab:
  def __init__(self):
    pass

class BoxerWindowSettings:

  # close the window and quit
  def delete_event(self, widget, event, data=None):
    gtk.main_quit()
    return False

  def __init__(self):
    # Create a new window
    self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    self.window.set_border_width(spacing)

    self.window.set_title("Boxer configuration editor")

    self.window.set_size_request(400, 300)

    self.window.connect("delete_event", self.delete_event)

    # The window has one top main region and a bottom region where the
    # ok/cancel buttons are
    self.window_versplit = gtk.VBox()
    self.window.add(self.window_versplit)

    # The window is split horizontally into two parts:
    # - on the left we have the zone where we can select the setting to change
    # - on the right we can actually manipulate the setting
    self.window_horsplit = gtk.HPaned()
    self.window_versplit.pack_start(self.window_horsplit, expand=True,
                                    fill=True, padding=0)

    self.window_button_ok = gtk.Button(label="_Ok")
    self.window_button_cancel = gtk.Button(label="_Cancel")

    self.window_butbox = gtk.HButtonBox()
    self.window_butbox.add(self.window_button_ok)
    self.window_butbox.add(self.window_button_cancel)
    self.window_butbox.set_layout(gtk.BUTTONBOX_END)
    self.window_butbox.set_spacing(spacing)
    debug()
    self.window_versplit.pack_start(self.window_butbox, expand=False,
                                    fill=False, padding=0)


    # We first define the right part, which is split vertically in two
    self.window_versplit2 = gtk.VBox(False, 4)
    self.window_horsplit.pack2(self.window_versplit2)

    # RIGHT PART: In the upper part we have a text entry, which is initially
    # filled with the current setting and the user can edit in order to change
    # it.
    self.window_entry = gtk.Entry()
    self.window_versplit2.pack_start(self.window_entry, expand=False,
                                    fill=False, padding=4)

    self.window_textview = gtk.TextView()
    self.window_textview.set_editable(False)
    self.window_textview.set_cursor_visible(False)
    self.window_textview.set_wrap_mode(gtk.WRAP_NONE)
    self.window_versplit2.pack_start(self.window_textview, expand=True,
                                     fill=True, padding=4)

    # create a TreeStore with one string column to use as the model
    self.treestore = gtk.TreeStore(str)

    piter = self.treestore.append(None, ['Box'])
    self.treestore.append(piter, ['Executable path'])

    piter = self.treestore.append(None, ['GUI'])
    self.treestore.append(piter, ['Size of point marker'])

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

    # make it searchable
    self.treeview.set_search_column(0)

    # Allow sorting on the column
    self.tvcolumn.set_sort_column_id(0)

    # Allow drag and drop reordering of rows
    self.treeview.set_reorderable(True)

    self.window_horsplit.pack1(self.treeview)

    self.window.show_all()

def main():
  gtk.main()

if __name__ == "__main__":
  tmp = BoxerWindowSettings()
  main()
