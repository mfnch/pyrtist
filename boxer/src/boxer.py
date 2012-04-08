# Copyright (C) 2008-2011
#  by Matteo Franchin (fnch@users.sourceforge.net)
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

# TODO (for later):
# - extend do/undo to refpoints
# - add show all/hide all button
# - configuration files should work as before
# - buffer geometry should be determined by memory available for buffer
# - multiple selection of points and transformation on them
#   (translation, rotation,...)
# - find and replace
# - rename RefPoint? (dangerous, should be assisted)
# - configuration window

# DONE IN VER 0.3.0:
# x added the Box assistant (color window, polygons, lines, etc)
# x load and save dialogs should remember last opened directory(independently)
# x remember last window configuration (size of main window)
# x allow cancel exit in window asking whether to save or not unsaved changes

# DONE IN VER 0.2.0:
# x it should be possible to terminate execution of running scripts
# x refpoints should have yellow color
# x paste button should work as before

# x initial script should have bounding box defined by two RefPoint-s
# x bounding box should be visible somehow
# x shade view when there are errors (drawing failed)
# x vertical placement of text editor and graphic view
#   Now it is [--] while we should allow [ | ]
#   There are four combinations [Text|Figure], [Figure|Text], etc.
# x resize reference points

import time
import os

import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import gtk.gdk
import gobject

import config
from config import threads_init, threads_enter, threads_leave
import docbase
import document
from exec_command import exec_command

from srcview import BoxSrcView
import boxmode
from assistant import Assistant, insert_char, rinsert_char
from toolbox import ToolBox
from editable import BoxEditableArea
from rotpaned import RotPaned

from dox.dox import Dox
from dox.browse import DoxBrowser

def debug():
  import sys
  from IPython.Shell import IPShellEmbed
  calling_frame = sys._getframe(1)
  IPShellEmbed([])(local_ns  = calling_frame.f_locals,
                   global_ns = calling_frame.f_globals)


def create_filechooser(parent, title="Choose a file", action=None,
                       buttons=None, add_default_filters=True):
  fc = gtk.FileChooserDialog(title=title,
                             parent=parent,
                             action=action,
                             buttons=buttons)
  if add_default_filters:
    flt_box_sources = gtk.FileFilter()
    flt_box_sources.set_name("Box sources")
    flt_box_sources.add_pattern("*.box")
    flt_all_files = gtk.FileFilter()
    flt_all_files.set_name("All files")
    flt_all_files.add_pattern("*")
    fc.add_filter(flt_box_sources)
    fc.add_filter(flt_all_files)

  return fc


class Settings(object):
  """Class used to save settings when entering/exiting the assistant modes."""

  def __init__(self):
    """Create a new setting object."""
    self.layers = []
    self.push()

  def push(self):
    self.layers.append({})

  def pop(self):
    self.layers.pop()

  def set_props(self, **props):
    """Set a new property."""
    # We could check whether the old value is the same and avoid setting
    # but don't think this will make much difference here
    layer = self.layers[-1]
    for key, value in props.iteritems():
      layer[key] = value

  def set_prop(self, name, value):
    """Equivalent to .set_prop("xyz", "value") is equivalent to
    .set_props(xyz="value")"""
    layer = self.layers[-1][name] = value

  def get_prop(self, name):
    for layer in reversed(self.layers):
      if name in layer:
        return layer[name]

    raise KeyError("Property not found in Settings object")


class Boxer(object):
  ui_info = \
    '''<ui>
      <menubar name='MenuBar'>
        <menu action='FileMenu'>
          <menuitem action='New'/>
          <menuitem action='Open'/>
          <menu action='ExamplesMenu'>
          </menu>
          <separator/>
          <menuitem action='Save'/>
          <menuitem action='SaveAs'/>
          <separator/>
          <menuitem action='Quit'/>
        </menu>
        <menu action='EditMenu'>
          <menuitem action='Undo'/>
          <menuitem action='Redo'/>
          <separator/>
          <menuitem action='Cut'/>
          <menuitem action='Copy'/>
          <menuitem action='Paste'/>
          <menuitem action='Delete'/>
        </menu>
        <menu action='LibraryMenu'>
          <menuitem action='ConfigureLibrary'/>
        </menu>
        <menu action='RunMenu'>
          <menuitem action='Execute'/>
          <menuitem action='Stop'/>
        </menu>
        <menu action='ViewMenu'>
          <menuitem action='RotateView'/>
          <menuitem action='BigRP'/>
          <menuitem action='SmallRP'/>
          <menuitem action='Remember'/>
          <menuitem action='Forget'/>
        </menu>
        <menu action='HelpMenu'>
          <menuitem action='DocBrowser'/>
          <menuitem action='About'/>
        </menu>
      </menubar>
      <toolbar  name='ToolBar'>
        <toolitem action='New'/>
        <toolitem action='Open'/>
        <toolitem action='Save'/>
        <separator/>
        <toolitem action='Execute'/>
        <toolitem action='Stop'/>
        <separator/>
        <toolitem action='ZoomIn'/>
        <toolitem action='ZoomOut'/>
        <toolitem action='Zoom100'/>
        <separator/>
        <toolitem action='PasteRP'/>
      </toolbar>
    </ui>'''

  def _init_menu_and_toolbar(self):
    # The window will contain a VBox as its main object: the menu bar is the
    # first object at the top of the VBox object
    vbox = gtk.VBox(homogeneous=False, spacing=0)
    mainwin = self.mainwin
    mainwin.add(vbox)

    # Use a gtk.UIManager instance to create menu and toolbar
    merge = gtk.UIManager()
    mainwin.set_data("ui-manager", merge)
    merge.insert_action_group(self._init_action_group(), 0)
    mainwin.add_accel_group(merge.get_accel_group())

    try:
      mergeid = merge.add_ui_from_string(self.ui_info)
    except gobject.GError, msg:
      print "building menus failed: %s" % msg

    # Retrieve the menu bar and the tool bar and add them at the top of the
    # VBox
    menubar = merge.get_widget("/MenuBar")
    vbox.pack_start(menubar, expand=False)    
    toolbar = merge.get_widget("/ToolBar")
    toolbar.set_tooltips(True)
    toolbar.set_show_arrow(False)

    self.menubutton_run_execute = merge.get_widget("/MenuBar/RunMenu/Execute")
    self.menubutton_run_stop = merge.get_widget("/MenuBar/RunMenu/Stop")
    self.toolbutton_run = merge.get_widget("/ToolBar/Execute")
    self.toolbutton_stop = merge.get_widget("/ToolBar/Stop")
    self.widget_pastenewbutton = merge.get_widget("/ToolBar/PasteRP")

    self.examplesmenu = mn = gtk.Menu()
    emn = merge.get_widget("/MenuBar/FileMenu/ExamplesMenu")
    emn.set_submenu(mn)

    # HBox containing the toolbar, the refpoint combobox, etc
    hbox = gtk.HBox(homogeneous=False, spacing=0)
    hbox.pack_start(toolbar, expand=False, fill=False)
    vbox2 = gtk.VBox()
    rhbox = self._init_toolbox_rhs()
    vbox2.pack_start(rhbox, expand=False, fill=False)
    hbox.pack_start(gtk.SeparatorToolItem(), expand=False)
    hbox.pack_start(vbox2, expand=False)

    # Finally add the toolbar to the VBox
    vbox.pack_start(hbox, expand=False)
    return vbox

  def _init_toolbox_rhs(self):
    # The right part of the toolbox
    rhbox = gtk.HBox(homogeneous=False, spacing=0)

    # The reference point combo box and show/hide button
    self.widget_refpoint_box = combo = gtk.ComboBoxEntry()
    self.widget_refpoint_entry = entry = combo.get_child()
    entry.connect("changed", self.refpoint_entry_changed)
    combo.set_tooltip_text(
      "The name of the next or selected reference point. You can enter "
      "text here to select reference points. You can use wildcards, such "
      "as 'point*'. '*' selects all reference points.")
    combo.set_size_request(100, -1)
    liststore = gtk.ListStore(gobject.TYPE_STRING)
    combo.set_model(liststore)
    combo.set_text_column(0)
    rhbox.pack_start(combo, expand=False)

    self.widget_refpoint_show = button = gtk.Button("show")
    button.connect("clicked", self.refpoint_show_clicked)


    button.set_tooltip_text("Show or hide the selected reference point. "
                            "Write 'point*' into the box to select "
                            "point1, point2, etc.")
    button.set_size_request(60, -1)
    rhbox.pack_start(button, expand=False)

    rhbox.pack_start(gtk.SeparatorToolItem(), expand=False)

    # The help entry and button in the toolbar
    self.widget_help_entry = entry = gtk.Entry()
    entry.set_tooltip_text("Write here a type you want to know about and "
                           "press [ENTER]")
    entry.set_size_request(100, -1)
    entry.connect("activate", self.on_help_entry_activate)
    rhbox.pack_start(entry, expand=False)
    button = gtk.Button("Help")
    button.connect("clicked", self.on_help_button_clicked)
    button.set_tooltip_text("Show the help browser")
    rhbox.pack_start(button, expand=False)
    return rhbox

  def _init_action_group(self):
    entries = (
      ("FileMenu", None, "_File"),
      ("New", gtk.STOCK_NEW, "_New", "<control>N",
       "Create a new Box program", self.menu_file_new),
      ("Open", gtk.STOCK_OPEN, "_Open", "<control>O",
       "Open an existing Box program", self.menu_file_open),
      ("ExamplesMenu", None, "_Examples"),
      ("Save", gtk.STOCK_SAVE, "_Save", "<control>S",
       "Save the current Box program", self.menu_file_save),
      ("SaveAs", gtk.STOCK_SAVE, "Save _As...", None,
       "Save to a file", self.menu_file_save_as),
      ("Quit", gtk.STOCK_QUIT, "_Quit", "<control>Q",
       "Quit Boxer", self.menu_file_quit),

      ("EditMenu", None, "_Edit"),
      ("Undo", gtk.STOCK_UNDO, "_Undo", "<control>Z",
       "Create a new Box program", self.menu_edit_undo),
      ("Redo", gtk.STOCK_REDO, "_Redo", "<control><shift>Z",
       "Create a new Box program", self.menu_edit_redo),
      ("Cut", gtk.STOCK_CUT, "Cu_t", "<control>X",
       "Create a new Box program", self.menu_edit_cut),
      ("Copy", gtk.STOCK_COPY, "_Copy", "<control>C",
       "Create a new Box program", self.menu_edit_copy),
      ("Paste", gtk.STOCK_PASTE, "_Paste", "<control>V",
       "Create a new Box program", self.menu_edit_paste),
      ("Delete", gtk.STOCK_DELETE, "_Delete", None,
       "Create a new Box program", self.menu_edit_delete),

      ("LibraryMenu", None, "_Library"),
      ("ConfigureLibrary", gtk.STOCK_PREFERENCES, "_Configure library", None,
       "Configure the Boxer library", self.menu_library_config),

      ("RunMenu", None, "_Run"),
      ("Execute", gtk.STOCK_EXECUTE, "_Execute", "<control>Return",
       "Execute the Box program", self.menu_run_execute),
      ("Stop", gtk.STOCK_STOP, "_Stop", "<control>BackSpace",
       "Stop a running Box program", self.menu_run_stop),

      ("ViewMenu", None, "_View"),
      ("RotateView", None, "_Rotate view", "<control>L",
       "Change the position of the text view in the window",
       self.menu_view_rotate),
      ("BigRP", None, "_Increase point marker size", "<control>plus",
       "Increase the size of the reference point markers",
       self.menu_view_inc_refpoint),
      ("SmallRP", None, "_Reduce point marker size", "<control>minus",
       "Reduce the size of the reference point markers",
       self.menu_view_dec_refpoint),
      ("Remember", None, "R_emember the current window size", None,
       ("Remember the current window size when launching Boxer "
        "the next time"), self.menu_view_remember_win_size),
      ("Forget", None, "_Forget the saved window size", None,
       "Use the default window size when Boxer starts",
       self.menu_view_forget_win_size),

      ("HelpMenu", None, "_Help"),
      ("DocBrowser", None, "_Documentation browser", "<control>H",
       "Show the Box documentation browser", self.menu_help_docbrowser),
      ("About", gtk.STOCK_ABOUT, "_About", None,
       "Show information about the program", self.menu_help_about),

      ("ZoomIn", gtk.STOCK_ZOOM_IN, "_Zoom In", None,
       "Zoom in", self.menu_zoom_in),
      ("ZoomOut", gtk.STOCK_ZOOM_OUT, "_Zoom Out", None,
       "Zoom in", self.menu_zoom_out),
      ("Zoom100", gtk.STOCK_ZOOM_100, "_See whole figure", None,
       "See whole figure", self.menu_zoom_norm),
    )

    toggle_entries = (
      ("PasteRP", gtk.STOCK_EDIT, "_Paste on click", None,
       "Paste the name of reference points whenever they are created",
       None),
    )

    # Create the menubar and toolbar
    action_group = gtk.ActionGroup("AppWindowActions")
    action_group.add_actions(entries)
    action_group.add_toggle_actions(toggle_entries)
    return action_group

  def delete_event(self, widget, event, data=None):
    return not self.ensure_file_is_saved()

  def raw_quit(self):
    """Called to quit the program."""
    self.editable_area.kill_drawer() # Terminate running processes if any
    self.config.save_configuration() # Save the configuration to file
    gtk.main_quit()

  def destroy(self, widget, data=None):
    self.raw_quit()

  def assume_file_is_saved(self):
    """Called just after saving a file, to communicate that this has just
    happened.
    """
    self.widget_srcbuf.set_modified(False)

  def update_title(self):
    """Update the title in the main window. Must be called when the file name
    changes or is set."""
    filename = self.filename
    if filename == None:
      filename = "New file (not saved)"

    self.mainwin.set_title("%s - Boxer" % filename)

  def get_main_source(self):
    """Return the content of the main textview (just a string)."""
    tb = self.widget_srcbuf
    return tb.get_text(tb.get_start_iter(), tb.get_end_iter())

  def set_main_source(self, text, not_undoable=None):
    """Set the content of the main textview from the string 'text'."""
    if not_undoable == None:
      not_undoable = self.has_srcview
    if not_undoable:
      self.widget_srcbuf.begin_not_undoable_action()
    self.widget_srcbuf.set_text(text)

    # Remove the "here" marker and put the cursor there!
    here_marker = docbase.marker_cursor_here
    si = self.widget_srcbuf.get_start_iter()
    found = si.forward_search(here_marker, gtk.TEXT_SEARCH_TEXT_ONLY)
    if found != None:
      mark0, mark1 = found
      self.widget_srcbuf.select_range(mark0, mark1)
      self.widget_srcbuf.delete_selection(True, True)

    if not_undoable:
      self.widget_srcbuf.end_not_undoable_action()

  def raw_file_new(self):
    """Start a new box program and set the content of the main textview."""
    from config import box_source_of_new
    d = self.editable_area.document
    d.load_from_str(box_source_of_new)

    self.editable_area.reset()
    self.widget_toolbox.exit_all_modes(force=True)
    self.set_main_source(d.get_user_code())
    self.filename = None
    self.assume_file_is_saved()
    self.update_title()
    self.editable_area.refresh()

  def raw_file_open(self, filename):
    """Load the file 'filename' into the textview."""
    d = self.editable_area.document
    try:
      d.load_from_file(filename)
    except Exception as the_exception:
      self.error('Error loading the file "%s". Details: "%s"'
                 % (filename, str(the_exception)))
      return
    finally:
      self.editable_area.reset()
      self.widget_toolbox.exit_all_modes(force=True)
      self.set_main_source(d.get_user_code())
      self.filename = filename
      self.assume_file_is_saved()
      self.update_title()
      self.editable_area.zoom_off()

  def raw_file_save(self, filename=None):
    """Save the textview content into the file 'filename'."""
    d = self.editable_area.document
    try:
      if filename == None:
        filename = self.filename
      d.set_user_code(self.get_main_source())
      d.save_to_file(filename)

    except Exception as exc:
      self.error("Error saving the file: %s" % str(exc))
      return False

    self.filename = filename
    self.update_title()
    self.assume_file_is_saved()
    return True

  def ensure_file_is_saved(self):
    """Give to the user a possibility of saving the work that otherwise would
    be discarded (by an "open file" a "new" or an "application close" command)
    Return True if the user decided Yes or No. Return False to cancel
    the action.
    """
    if not self.widget_srcbuf.get_modified():
      return True

    msg = "The file contains unsaved changes. Do you want to save it now?"
    md = gtk.MessageDialog(parent=self.mainwin,
                           type=gtk.MESSAGE_QUESTION,
                           message_format=msg,
                           buttons=gtk.BUTTONS_NONE)
    md.add_buttons(gtk.STOCK_YES, gtk.RESPONSE_YES,
                   gtk.STOCK_NO, gtk.RESPONSE_NO,
                   gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL)

    response = md.run()
    md.destroy()

    if response == gtk.RESPONSE_CANCEL:
      return False

    elif response == gtk.RESPONSE_YES:
      self.menu_file_save(None)
      return (not self.widget_srcbuf.get_modified())

    else:
      return True

  def error(self, msg):
    md = gtk.MessageDialog(parent=self.mainwin,
                           type=gtk.MESSAGE_ERROR,
                           message_format=msg,
                           buttons=gtk.BUTTONS_OK)
    md.run()
    md.destroy()

  def menu_file_new(self, image_menu_item):
    if not self.ensure_file_is_saved():
      return
    self.raw_file_new()

  def menu_file_open(self, image_menu_item):
    """Ivoked to open a file. Shows the dialog to select the file."""
    if not self.ensure_file_is_saved():
      return

    if self.dialog_fileopen == None:
      self.dialog_fileopen = \
        create_filechooser(self.mainwin,
                           title="Open Box program",
                           action=gtk.FILE_CHOOSER_ACTION_OPEN,
                           buttons=("_Cancel", gtk.RESPONSE_CANCEL,
                                    "_Open", gtk.RESPONSE_OK))

    fc = self.dialog_fileopen
    response = fc.run()
    filename = fc.get_filename() if response == gtk.RESPONSE_OK else None
    fc.hide()

    if filename != None:
      self.raw_file_open(filename)

  def menu_file_save(self, image_menu_item):
    """Ivoked to save a file whose name has been already assigned."""
    if self.filename != None:
      self.raw_file_save()
    else:
      self.menu_file_save_as(image_menu_item)

  def menu_file_save_as(self, image_menu_item):
    """Ivoked to save a file. Shows the dialog to select the file name."""

    if self.dialog_filesave == None:
      self.dialog_filesave = \
        create_filechooser(self.mainwin,
                           title="Save Box program",
                           action=gtk.FILE_CHOOSER_ACTION_SAVE,
                           buttons=("_Cancel", gtk.RESPONSE_CANCEL,
                                    "_Save", gtk.RESPONSE_OK))

    fc = self.dialog_filesave
    response = fc.run()
    do_save_it = (response == gtk.RESPONSE_OK)
    filename = fc.get_filename() if do_save_it else None
    fc.hide()

    if filename != None:
      self.raw_file_save(filename)

  def menu_file_quit(self, image_menu_item):
    """Called on file->quit to quit the program."""
    if self.ensure_file_is_saved():
      self.raw_quit()

  def show_undo_redo_warning(self):
    if self._undo_redo_warning_shown:
      return

    self._undo_redo_warning_shown = True
    self.error("Cannot undo/redo. Please install python-gtksourceview (or "
               "python-gtksourceview2). This will give you undo-redo "
               "capabilities, highlighting of the sources and other nice "
               "features.")

  def menu_edit_undo(self, image_menu_item):
    """Called on a CTRL+Z or menu->undo."""
    try:
      if self.widget_srcbuf.can_undo():
        self.widget_srcbuf.undo()

    except:
      self.show_undo_redo_warning()

  def menu_edit_redo(self, image_menu_item):
    """Called on a CTRL+SHIFT+Z or menu->redo."""
    try:
      if self.widget_srcbuf.can_redo():
        self.widget_srcbuf.redo()

    except:
      self.show_undo_redo_warning()

  def menu_edit_cut(self, image_menu_item):
    """Called on a CTRL+X (cut) command."""
    self.widget_srcbuf.cut_clipboard(self.clipboard,
                                     self.widget_srcview.get_editable())

  def menu_edit_copy(self, image_menu_item):
    """Called on a CTRL+C (copy) command."""
    self.widget_srcbuf.copy_clipboard(self.clipboard)

  def menu_edit_paste(self, image_menu_item):
    """Called on a CTRL+V (paste) command."""
    self.widget_srcbuf.paste_clipboard(self.clipboard, None,
                                       self.widget_srcview.get_editable())

  def menu_edit_delete(self, image_menu_item):
    """Called menu edit->delete command."""
    self.widget_srcbuf.delete_selection(True,
                                        self.widget_srcview.get_editable())

  def menu_run_execute(self, image_menu_item):
    """Called by menu run->execute command."""
    self.update_draw_area()

  def menu_library_config(self, image_menu_item):
    """Called by menu library->configure command."""
    pass

  def update_draw_area(self, only_if_quick=False):
    """Update the draw area (executing the Box program)."""
    self.editable_area.refresh()

  def menu_run_stop(self, image_menu_item):
    if self.box_killer != None:
      self.box_killer()

  def menu_view_rotate(self, _):
    self.paned.rotate()
    self.config.set("GUI", "rotation", str(self.paned.rotation))

  def menu_view_inc_refpoint(self, _):
    rps = self.config.getint("GUIView", "refpoint_size")
    new_rps = min(rps + 1, 20)
    self.config.set("GUIView", "refpoint_size", str(new_rps))
    self.editable_area.set_config(refpoint_size=new_rps)
    self.editable_area.queue_draw()

  def menu_view_dec_refpoint(self, _):
    rps = self.config.getint("GUIView", "refpoint_size")
    new_rps = max(rps - 1, 2)
    self.config.set("GUIView", "refpoint_size", str(new_rps))
    self.editable_area.set_config(refpoint_size=new_rps)
    self.editable_area.queue_draw()

  def menu_view_remember_win_size(self, _):
    size_str = "%dx%d" % tuple(self.mainwin.get_size())
    self.config.set("GUI", "window_size", size_str)

  def menu_view_forget_win_size(self, _):
    self.config.remove_option("GUI", "window_size")

  def menu_zoom_in(self, image_menu_item):
    self.editable_area.zoom_in()

  def menu_zoom_out(self, image_menu_item):
    self.editable_area.zoom_out()

  def menu_zoom_norm(self, image_menu_item):
    self.editable_area.zoom_off()

  def _set_box_killer(self, killer):
    killer_given = (killer != None)
    self.toolbutton_stop.set_sensitive(killer_given)
    self.menubutton_run_stop.set_sensitive(killer_given)
    self.toolbutton_run.set_sensitive(not killer_given)
    self.menubutton_run_execute.set_sensitive(not killer_given)
    self.box_killer = killer

  def _out_textview_refresh(self, text, force=False):
    t = time.time()
    if force == False:
      if self.out_textbuffer_last_update_time != None:
        t0 = self.out_textbuffer_last_update_time
        if t - t0 < self.out_textbuffer_update_time:
          return text

    has_some_text = (len(text.strip()) > 0)
    if len(text) > self.out_textbuffer_capacity:
      text = text[-self.out_textbuffer_capacity:]
    self.out_textview_expander.set_expanded(has_some_text)
    self.out_textbuffer.set_text(text)
    self.out_textbuffer_last_update_time = t
    return text

  def menu_help_about(self, image_menu_item):
    """Called menu help->about command."""
    ad = gtk.AboutDialog()
    import info
    ad.set_name(info.name)
    ad.set_version(info.version_string)
    ad.set_comments(info.comment)
    ad.set_license(info.license)
    ad.set_authors(info.authors)
    ad.set_website(info.website)
    ad.set_logo(None)

    ad.run()
    ad.destroy()

  def menu_help_docbrowser(self, _):
    self._init_doxbrowser()
    if self.dialog_dox_browser:
      self.dialog_dox_browser.show()

  def refpoint_entry_changed(self, _):
    self.refpoint_show_update()

  def notify_refpoint_new(self, obj, rp):
    liststore = self.widget_refpoint_box.get_model()
    liststore.insert(-1, row=(rp.name,))

  def notify_refpoint_del(self, obj, rp):
    liststore = self.widget_refpoint_box.get_model()
    for item in liststore:
      if liststore.get_value(item.iter, 0) == rp.name:
        liststore.remove(item.iter)
        return

  def refpoint_show_update(self):
    selection = self.widget_refpoint_entry.get_text()
    d = self.editable_area.document
    ratio = d.refpoints.get_visible_ratio(selection)
    if ratio < 0.5:
      label, show = "Show", True
    elif ratio > 0.5:
      label, show = "Hide", False
    else:
      return
    self.widget_refpoint_show.set_label(label)

  def refpoint_show_clicked(self, button):
    do_show = (button.get_label().lower() == "show")
    selection = self.widget_refpoint_entry.get_text()
    self.editable_area.refpoints_set_visibility(selection, do_show)
    self.refpoint_show_update()

  def on_help_button_clicked(self, _):
    topic = self.widget_help_entry.get_text().strip()
    if self.dialog_dox_browser != None:
      self.dialog_dox_browser.show(topic=topic if len(topic) > 0 else None)

  def on_help_entry_activate(self, _):
    self.on_help_button_clicked(_)

  def should_paste_on_new(self):
    """Return true if the name of the reference points should be pasted
    to the current edited source when they are created.
    """
    return self.widget_pastenewbutton.get_active()

  def set_props(self, **props):
    for key, value in props.iteritems():
      if key == "paste_point":
        self.widget_pastenewbutton.set_active(value)
      else:
        self.settings.set_prop(key, value)

  def do(self, **props):
    for key, value in props.iteritems():
      if key == "push_settings":
        self.settings.push()
      elif key == "pop_settings":
        self.settings.pop()
      elif key == "update":
        self.update_draw_area(only_if_quick=True)

  def srcview_tooltip(self, word):
    db = self.dialog_dox_browser

    if word[0].isupper():
      if db != None:
        desc = db.get_brief_desc(word)
        #if desc:
        #  self.widget_help_entry.set_text(word)
        return desc

    return None

  def _init_doxbrowser(self):
    dox_browser = self.dialog_dox_browser
    if dox_browser == None:
      try:
        dox_path = self.editable_area.document.box_query("BUILTIN_PKG_PATH")
      except:
        dox_path = None

      if dox_path:
        dox = Dox()
        dox.read_recursively(dox_path)
        tree = dox.tree
        tree.process()
        self.dialog_dox_browser = dox_browser = DoxBrowser(dox)

  def __init__(self, filename=None, box_exec=None):
    self.filename = filename    
    self.config = config.get_configuration()
    if box_exec != None:
      self.config.set("Box", "exec", box_exec)

    self.settings = Settings()
    self.settings.set_props(update_on_paste=False)

    # Dialogues
    self.dialog_fileopen = None
    self.dialog_filesave = None
    self.dialog_dox_browser = None

    self._undo_redo_warning_shown = False
    self.box_killer = None

    self.button_left = self.config.getint("Behaviour", "button_left")
    self.button_center = self.config.getint("Behaviour", "button_center")
    self.button_right = self.config.getint("Behaviour", "button_right")

    # Create the main window
    self.mainwin = mainwin = gtk.Window()
    mainwin.connect("destroy", self.destroy)
    mainwin.connect("delete_event", self.delete_event)
    self.update_title()

    # Set the last saved window size, if any
    try:
      wsx, wsy = map(int, self.config.get("GUI", "window_size").split("x", 1))
    except:
      wsx, wsy = (600, 600)
    mainwin.set_default_size(wsx, wsy)

    # Create the menu and toolbar
    vbox = self._init_menu_and_toolbar()

    self.out_textview = outtv = gtk.TextView()
    outtv.set_size_request(10, 75)
    outtv.set_editable(False)
    outtv.set_cursor_visible(False)
    self.out_textbuffer = outtv.get_buffer()
    self.out_textview_expander = outexp = gtk.Expander(label="Box output:")
    outsw = gtk.ScrolledWindow()
    outsw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
    outsw.add(outtv)
    outexp.add(outsw)

    self.out_textbuffer_last_update_time = None
    self.out_textbuffer_update_time = \
      self.config.getfloat('Box', 'stdout_update_delay')
    self.out_textbuffer_capacity = \
      int(1024*self.config.getfloat('Box', 'stdout_buffer_size'))

    #-------------------------------------------------------------------------
    # Below we setup the main window

    # Create the editable area and do all the wiring
    cfg = {"box_executable": self.config.get("Box", "exec"),
           "refpoint_size": self.config.getint("GUIView", "refpoint_size")}
    self.editable_area = editable_area = BoxEditableArea(config=cfg)
    editable_area.set_callback("zoomablearea_got_killer", self._set_box_killer)
    editable_area.set_callback("refpoint_append", self.notify_refpoint_new)
    editable_area.set_callback("refpoint_remove", self.notify_refpoint_del)

    def refpoint_press_middle(_, rp):
      tb = self.widget_srcbuf
      tb.begin_user_action()
      insert_char(tb)
      tb.insert_at_cursor(rp.name)
      tb.end_user_action()

      if self.settings.get_prop("update_on_paste"):
        self.update_draw_area(only_if_quick=True)
    editable_area.set_callback("refpoint_press_middle", refpoint_press_middle)

    def new_rp(_, rp):
      if self.should_paste_on_new():
        refpoint_press_middle(None, rp)
    editable_area.set_callback("refpoint_new", new_rp)

    def get_next_refpoint(doc):
      return self.widget_refpoint_entry.get_text()
    editable_area.set_callback("get_next_refpoint_name", get_next_refpoint)

    def set_next_refpoint_name(doc, rp_name):
      self.widget_refpoint_entry.set_text(rp_name)
    editable_area.set_callback("set_next_refpoint_name",
                               set_next_refpoint_name)

    def set_next_refpoint(doc, rp):
      self.widget_refpoint_entry.set_text(rp.name)
    editable_area.set_callback("refpoint_pick", set_next_refpoint)

    box_output = [None]
    def box_document_execute(doc, preamble, out_fn, exit_fn):
      doc.set_user_code(self.get_main_source())
      box_output[0] = ""
    editable_area.set_callback("box_document_execute", box_document_execute)

    def box_exec_output(s, force=False):
      box_output[0] += s
      box_output[0] = self._out_textview_refresh(box_output[0], force=force)
    editable_area.drawer.set_output_function(box_exec_output)

    def box_document_executed(doc):
      box_exec_output("", force=True)
    editable_area.set_callback("box_document_executed", box_document_executed)

    # Create the scrolled window containing the box-draw editable area
    scroll_win1 = gtk.ScrolledWindow()
    scroll_win1.add(editable_area)

    # Create the text view
    self.part_srcview = part_srcview = \
        BoxSrcView(use_gtksourceview=True,
                   quickdoc=self.srcview_tooltip)
    self.has_srcview = (self.part_srcview.mode > 0)
    self.widget_srcview = srcview = part_srcview.view
    self.widget_srcbuf = srcbuf = part_srcview.buf

    # Put the sourceview inside a scrolled window
    svsw = gtk.ScrolledWindow()
    svsw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
    svsw.add(srcview)

    # Put source and output view inside a single widget
    src_and_out_views = gtk.VBox()
    src_and_out_views.pack_start(svsw, expand=True, fill=True)
    src_and_out_views.pack_start(self.out_textview_expander,
                                 expand=False, fill=True)

    # Create the scrolled window containing the text views (source + output)
    scroll_win2 = gtk.ScrolledWindow()
    scroll_win2.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
    scroll_win2.add_with_viewport(src_and_out_views)

    # Put them together in the RotView widget
    view_rot = self.config.getint("GUI", "rotation")
    self.paned = paned = RotPaned(scroll_win1, scroll_win2,
                                  rotation=view_rot)

    # Create the statusbar
    self.statusbar = sbar = gtk.Statusbar()

    # Create the assistant and the ToolBox
    self.assistant = astn = Assistant(boxmode.main_mode)
    self.widget_toolbox = tbox = ToolBox(astn, icon_path=config.icon_path)
    astn.set_statusbar(sbar)
    astn.set_textview(srcview)
    astn.set_textbuffer(srcbuf)
    astn.set_window(mainwin)
    astn.set_gui(self)

    # Create the layout in form of HBox-es and VBox-es and pack all widgets
    self.widget_vbox1 = vb1 = vbox
    self.widget_hbox1 = hb1 = gtk.HBox(spacing=0)

    vb1.pack_start(hb1, expand=True)
    vb1.pack_start(sbar, expand=False)
    hb1.pack_start(tbox, expand=False)
    hb1.pack_start(gtk.VSeparator(), expand=False)
    hb1.pack_start(paned.get_container(), expand=True)

    # Initialize the documentation browser
    self._init_doxbrowser()

    #-------------------------------------------------------------------------

    # Set the name for the first reference point
    set_next_refpoint_name(self.editable_area.document, "p1")

    # try to set the default font
    try:
      from pango import FontDescription
      font = FontDescription(self.config.get("GUI", "font"))
      self.widget_srcview.modify_font(font)

    except:
      pass

    self.clipboard = gtk.Clipboard()

    # Find examples and populate the menu File->Examples
    self._fill_example_menu()

    # Show the window
    mainwin.show_all()

    # Set a template program to start with...
    if filename == None:
      self.raw_file_new()
    else:
      self.raw_file_open(filename)

    # Now set the focus on the text view
    self.widget_srcview.grab_focus()

  def _fill_example_menu(self):
    """Populate the example submenu File->Examples"""
    def create_callback(filename):
      def save_file(menuitem):
        if not self.ensure_file_is_saved(): return
        self.raw_file_open(filename)
      return save_file

    example_files = config.get_example_files()
    example_files.sort()
    i = 0
    for example_file in example_files:
      example_file_basename = os.path.basename(example_file)
      example_menuitem = gtk.MenuItem(label=example_file_basename,
                                      use_underline=False)
      callback = create_callback(example_file)
      example_menuitem.connect("activate", callback)
      self.examplesmenu.attach(example_menuitem, 0, 1, i, i+1)
      example_menuitem.show()
      i += 1

def run(filename=None, box_exec=None):
  threads_init()
  threads_enter()

  main_window = Boxer(filename=filename, box_exec=box_exec)
  gtk.main()

  threads_leave()

