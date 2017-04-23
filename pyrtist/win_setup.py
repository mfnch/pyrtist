# Copyright (C) 2017 by Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Pyrtist.
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

#!/usr/bin/env python

# example basictreeview.py

import pygtk
pygtk.require('2.0')
import gtk

import gui.config as config
from gui.boxer import debug

spacing = 6

class ConfigTab(object):
  def __init__(self):
    pass

class BoxerWindowSettings(object):
  def __init__(self, size=(600, 400)):
    # Create a new window
    self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    self.window.set_border_width(spacing)
    self.window.set_title("Boxer configuration editor")
    self.window.set_size_request(*size)

    self.window.connect("delete_event", self._on_delete_event)

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
    self.window_versplit.pack_start(self.window_butbox, expand=False,
                                    fill=False, padding=0)

    # We first define the right part, which is split vertically in two
    self.window_versplit2 = gtk.VBox(False, 4)
    self.window_horsplit.pack2(self.window_versplit2)

    # RIGHT PART: In the upper part we have a text entry, which is initially
    # filled with the current setting and the user can edit in order to change
    # it.
    self.window_label = wd = gtk.Label("No option selected.")
    wd.set_single_line_mode(False)
    wd.set_line_wrap(True)
    wd.set_alignment(0, 0)
    
    self.window_versplit2.pack_start(wd, expand=False, fill=False, padding=0)

    self.window_entry = gtk.Entry()
    self.window_versplit2.pack_start(self.window_entry, expand=False,
                                    fill=False, padding=0)

    self.window_textview = gtk.TextView()
    self.window_textview.set_editable(False)
    self.window_textview.set_cursor_visible(False)
    self.window_textview.set_wrap_mode(gtk.WRAP_NONE)
    self.window_versplit2.pack_start(self.window_textview, expand=True,
                                     fill=True, padding=4)

    # create a TreeStore with one string column to use as the model
    self.treestore = gtk.TreeStore(str)

    self.config = cfg = config.get_configuration()
    sections = cfg.get_sections()
    for section in sections:
      piter = self.treestore.append(None, [section])
      options = cfg.get_options(section)
      for option in options:
        self.treestore.append(piter, [option])

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
    self.window_scrolledwin = scrolledwin = gtk.ScrolledWindow()
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
    self._apply_changes()
    self.config.save_configuration()
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
      self._show_new_tab(section, option)

  def _save_tab_settings(self):
    entry = self.window_entry
    initial_val = self.option_initial_value
    if initial_val != None:
      cur_val = entry.get_text()
      if cur_val != initial_val:
        sect = self.option_changed_vals.setdefault(self.option_section, {})
        sect[self.option_name] = cur_val
    
  def _show_new_tab(self, section, option):
    self._save_tab_settings()

    desc = self.config.get_desc(section, option)
    range_str = desc.get_range()

    value = self.option_changed_vals.get(section, {}).get(option, None)
    if value == None:
      value = self.config.get(section, option)
    value = str(desc.get(value))

    entry = self.window_entry
    label = self.window_label
    opt_str = "%s.%s: %s" % (section, option, range_str)
    label.set_text("%s\n%s" % (opt_str, desc))
    entry.set_text(value)
    entry.set_tooltip_text(range_str)

    self.option_section = section
    self.option_name = option
    self.option_initial_value = value

  def _apply_changes(self):
    self._save_tab_settings()
    for section, section_dict in self.option_changed_vals.iteritems():
      for option, val in section_dict.iteritems():
        desc = self.config.get_desc(section, option)
        self.config.set(section, option, str(desc.set(val)))


def main():
  gtk.main()

if __name__ == "__main__":
  tmp = BoxerWindowSettings()
  main()
