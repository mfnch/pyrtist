# Copyright (C) 2008, 2009, 2010
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


# TODO (for v 0.2.0):
# x it should be possible to terminate execution of running scripts
# x refpoints should have yellow color
# x paste button should work as before

# x initial script should have bounding box defined by two RefPoint-s
# x bounding box should be visible somehow
# x shade view when there are errors (drawing failed)
# x vertical placement of text editor and graphic view
#   Now it is [--] while we should allow [ | ]
#   There are four combinations [Text|Figure], [Figure|Text], etc.

# TODO (for later):
# - configuration files should work as before
# - allow automatic placement (considers the shape of the figure
#   and decide what is more appropriate)
# - remember last window configuration (size of main window)
# - extend do/undo to refpoints
# - buffer geometry should be determined by memory available for buffer
# - load and save dialogs should remember last opened directory(independently)
# - drawing tools (color window, polygons, lines, etc)
# - multiple selection of points and transformation on them
#   (translation, rotation,...)
# - find and replace
# - rename RefPoint? (dangerous, should be assisted)
# - configuration window

import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import gtk.gdk
import os, os.path
import time

import config
import document
from exec_command import exec_command

from editable import BoxEditableArea
from rotpaned import RotPaned

def debug():
  import sys
  from IPython.Shell import IPShellEmbed
  calling_frame = sys._getframe(1)
  IPShellEmbed([])(local_ns  = calling_frame.f_locals,
                   global_ns = calling_frame.f_globals)

class Boxer:
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
    self.textbuffer.set_modified(False)

  def update_title(self):
    """Update the title in the main window. Must be called when the file name
    changes or is set."""
    filename = self.filename
    if filename == None:
      filename = "New file (not saved)"

    self.mainwin.set_title("%s - Boxer" % filename)

  def get_main_source(self):
    """Return the content of the main textview (just a string)."""
    tb = self.textbuffer
    return tb.get_text(tb.get_start_iter(), tb.get_end_iter())

  def set_main_source(self, text, not_undoable=None):
    """Set the content of the main textview from the string 'text'."""
    if not_undoable == None:
      not_undoable = self.has_textview
    if not_undoable: self.textbuffer.begin_not_undoable_action()
    self.textbuffer.set_text(text)

    # Remove the "here" marker and put the cursor there!
    here_marker = self.config.get("Internals", "src_marker_cursor_here")
    si = self.textbuffer.get_start_iter()
    found = si.forward_search(here_marker, gtk.TEXT_SEARCH_TEXT_ONLY)
    if found != None:
      mark0, mark1 = found
      self.textbuffer.select_range(mark0, mark1)
      self.textbuffer.delete_selection(True, True)

    if not_undoable: self.textbuffer.end_not_undoable_action()

  def raw_file_new(self):
    """Start a new box program and set the content of the main textview."""
    from config import box_source_of_new
    d = self.editable_area.document
    d.load_from_str(box_source_of_new)

    self.editable_area.kill_drawer()
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
      self.editable_area.kill_drawer()
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

    except:
      self.error("Error saving the file")
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
    if not self.textbuffer.get_modified(): return True
    msg = "The file contains unsaved changes. Do you want to save it now?"
    md = gtk.MessageDialog(parent=self.mainwin,
                           type=gtk.MESSAGE_QUESTION,
                           message_format=msg,
                           buttons=gtk.BUTTONS_YES_NO)
    response = md.run()
    md.destroy()
    if response == gtk.RESPONSE_YES:
      self.menu_file_save(None)
    return True

  def error(self, msg):
    md = gtk.MessageDialog(parent=self.mainwin,
                           type=gtk.MESSAGE_ERROR,
                           message_format=msg,
                           buttons=gtk.BUTTONS_OK)
    md.run()
    md.destroy()

  def menu_file_new(self, image_menu_item):
    if not self.ensure_file_is_saved(): return
    self.raw_file_new()

  def _add_filters(self, fc):
    flt_box_sources = gtk.FileFilter()
    flt_box_sources.set_name("Box sources")
    flt_box_sources.add_pattern("*.box")
    flt_all_files = gtk.FileFilter()
    flt_all_files.set_name("All files")
    flt_all_files.add_pattern("*")
    fc.add_filter(flt_box_sources)
    fc.add_filter(flt_all_files)

  def menu_file_open(self, image_menu_item):
    """Ivoked to open a file. Shows the dialog to select the file."""
    if not self.ensure_file_is_saved(): return
    fc = gtk.FileChooserDialog(title="Open Box program",
                               parent=self.mainwin,
                               action=gtk.FILE_CHOOSER_ACTION_OPEN,
                               buttons=("_Cancel", gtk.RESPONSE_CANCEL,
                                        "_Open", gtk.RESPONSE_OK))
    self._add_filters(fc)

    response = fc.run()
    filename = None
    if response == gtk.RESPONSE_OK:
      filename = fc.get_filename()
    fc.destroy()
    if filename != None: self.raw_file_open(filename)

  def menu_file_save(self, image_menu_item):
    """Ivoked to save a file whose name has been already assigned."""
    if self.filename != None:
      self.raw_file_save()
    else:
      self.menu_file_save_with_name(image_menu_item)

  def menu_file_save_with_name(self, image_menu_item):
    """Ivoked to save a file. Shows the dialog to select the file name."""
    fc = gtk.FileChooserDialog(title="Save Box program",
                               parent=self.mainwin,
                               action=gtk.FILE_CHOOSER_ACTION_SAVE,
                               buttons=("_Cancel", gtk.RESPONSE_CANCEL,
                                        "_Save", gtk.RESPONSE_OK))
    self._add_filters(fc)

    response = fc.run()
    filename = None
    if response == gtk.RESPONSE_OK:
      filename = fc.get_filename()
    fc.destroy()
    if filename != None: self.raw_file_save(filename)

  def menu_file_quit(self, image_menu_item):
    """Called on file->quit to quit the program."""
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
      if self.textbuffer.can_undo():
        self.textbuffer.undo()

    except:
      self.show_undo_redo_warning()

  def menu_edit_redo(self, image_menu_item):
    """Called on a CTRL+SHIFT+Z or menu->redo."""
    try:
      if self.textbuffer.can_redo():
        self.textbuffer.redo()

    except:
      self.show_undo_redo_warning()

  def menu_edit_cut(self, image_menu_item):
    """Called on a CTRL+X (cut) command."""
    self.textbuffer.cut_clipboard(self.clipboard,
                                  self.textview.get_editable())

  def menu_edit_copy(self, image_menu_item):
    """Called on a CTRL+C (copy) command."""
    self.textbuffer.copy_clipboard(self.clipboard)

  def menu_edit_paste(self, image_menu_item):
    """Called on a CTRL+V (paste) command."""
    self.textbuffer.paste_clipboard(self.clipboard,
                                    None,
                                    self.textview.get_editable())

  def menu_edit_delete(self, image_menu_item):
    """Called menu edit->delete command."""
    self.textbuffer.delete_selection(True,
                                     self.textview.get_editable())

  def menu_run_execute(self, image_menu_item):
    """Called by menu run->execute command."""
    self.editable_area.refresh()

  def menu_run_stop(self, image_menu_item):
    if self.box_killer != None:
      self.box_killer()

  def menu_view_rotate(self, _):
    self.paned.rotate()
    self.config.set("GUI", "rotation", str(self.paned.rotation))

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
      label, show = "show", True
    elif ratio > 0.5:
      label, show = "hide", False
    else:
      return
    self.widget_refpoint_show.set_label(label)

  def refpoint_show_clicked(self, button):
    do_show = (button.get_label() == "show")
    selection = self.widget_refpoint_entry.get_text()
    self.editable_area.refpoints_set_visibility(selection, do_show)
    self.refpoint_show_update()

  def should_paste_on_new(self):
    """Return true if the name of the reference points should be pasted
    to the current edited source when they are created.
    """
    return self.pastenewbutton.get_active()

  def __init__(self, gladefile="boxer.glade", filename=None, box_exec=None):
    self.config = config.get_configuration()
    if box_exec != None:
      self.config.set("Box", "exec", box_exec)

    self._undo_redo_warning_shown = False
    self.box_killer = None

    self.button_left = self.config.getint("Behaviour", "button_left")
    self.button_center = self.config.getint("Behaviour", "button_center")
    self.button_right = self.config.getint("Behaviour", "button_right")

    self.gladefile = config.glade_path(gladefile)
    self.boxer = gtk.glade.XML(self.gladefile, "boxer")
    self.mainwin = self.boxer.get_widget("boxer")
    self.textview = self.boxer.get_widget("textview")
    self.textbuffer = textbuffer = self.textview.get_buffer()
    self.out_textview = self.boxer.get_widget("outtextview")
    self.out_textbuffer = self.out_textview.get_buffer()
    self.out_textview_expander = self.boxer.get_widget("outtextview_expander")
    self.out_textbuffer_last_update_time = None
    self.out_textbuffer_update_time = \
      self.config.getfloat('Box', 'stdout_update_delay')
    self.out_textbuffer_capacity = \
      int(1024*self.config.getfloat('Box', 'stdout_buffer_size'))

    self.examplesmenu = self.boxer.get_widget("menu_file_examples")
    self.pastenewbutton = self.boxer.get_widget("toolbutton_pastenew")
    self.toolbutton_run = self.boxer.get_widget("toolbutton_run")
    self.toolbutton_stop = self.boxer.get_widget("toolbutton_stop")
    self.menubutton_run_execute = self.boxer.get_widget("run_execute")
    self.menubutton_run_stop = self.boxer.get_widget("run_stop")

    self.widget_refpoint_box = self.boxer.get_widget("refpoint_box")
    self.widget_refpoint_entry = self.boxer.get_widget("refpoint_entry")
    self.widget_refpoint_show = self.boxer.get_widget("refpoint_show")

    import gobject
    liststore = gtk.ListStore(gobject.TYPE_STRING)
    self.widget_refpoint_box.set_model(liststore)
    self.widget_refpoint_box.set_text_column(0)

    # Create the editable area and do all the wiring
    cfg = {"box_executable": self.config.get("Box", "exec"),
           "refpoint_size": self.config.getint("GUIView", "refpoint_size")}
    self.editable_area = editable_area = BoxEditableArea(config=cfg)
    editable_area.set_callback("zoomablearea_got_killer", self._set_box_killer)
    editable_area.set_callback("refpoint_append", self.notify_refpoint_new)
    editable_area.set_callback("refpoint_remove", self.notify_refpoint_del)

    def refpoint_press_middle(_, rp):
      self.textbuffer.insert_at_cursor("%s, " % rp.name)
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

    # Create the scrolled window
    scroll_win = gtk.ScrolledWindow()
    scroll_win.add(editable_area)

    # Add the scrolled window to the alignment widget
    vpaned = self.boxer.get_widget("vpaned")
    for child in vpaned.get_children():
      vpaned.remove(child)

    view_rot = self.config.getint("GUI", "rotation")
    sw = self.boxer.get_widget("srcview_scrolledwindow")
    self.paned = paned = RotPaned(scroll_win, sw, rotation=view_rot)

    # Setup the main HBox
    container = self.boxer.get_widget("vbox1")
    c1, c2, cc, c3 = children = container.get_children()
    for child in children:
      container.remove(child)
    container.pack_start(c1, expand=False)
    container.pack_start(c2, expand=False)
    container.pack_start(paned.get_container())
    container.pack_start(c3, expand=False)

    container.show_all()

    # Set the name for the first reference point
    set_next_refpoint_name(self.editable_area.document, "gui1")



    self.filename = None

    dic = {"on_boxer_destroy": self.destroy,
           "on_boxer_delete_event": self.delete_event,
           "on_file_new_activate": self.menu_file_new,
           "on_file_open_activate": self.menu_file_open,
           "on_file_save_activate": self.menu_file_save,
           "on_file_save_with_name_activate": self.menu_file_save_with_name,
           "on_file_quit_activate": self.menu_file_quit,
           "on_edit_undo_activate": self.menu_edit_undo,
           "on_edit_redo_activate": self.menu_edit_redo,
           "on_edit_cut_activate": self.menu_edit_cut,
           "on_edit_copy_activate": self.menu_edit_copy,
           "on_edit_paste_activate": self.menu_edit_paste,
           "on_edit_delete_activate": self.menu_edit_delete,
           "on_run_execute_activate": self.menu_run_execute,
           "on_run_stop_activate": self.menu_run_stop,
           "on_view_rotate_activate": self.menu_view_rotate,
           "on_help_about_activate": self.menu_help_about,
           "on_toolbutton_new": self.menu_file_new,
           "on_toolbutton_open": self.menu_file_open,
           "on_toolbutton_save": self.menu_file_save,
           "on_toolbutton_run": self.menu_run_execute,
           "on_toolbutton_stop": self.menu_run_stop,
           "on_toolbutton_zoom_in": self.menu_zoom_in,
           "on_toolbutton_zoom_out": self.menu_zoom_out,
           "on_toolbutton_zoom_norm": self.menu_zoom_norm,
           "on_refpoint_entry_changed": self.refpoint_entry_changed,
           "on_refpoint_show_clicked": self.refpoint_show_clicked}
    self.boxer.signal_autoconnect(dic)

    # Replace the TextView with a SourceView, if possible...
    self.has_textview = False
    try:
      import gtksourceview2 as gtksourceview
      srcbuf = gtksourceview.Buffer()
      langman = gtksourceview.LanguageManager()
      lang = langman.get_language("c")
      srcbuf.set_language(lang)
      srcbuf.set_highlight_syntax(True)
      srcview = gtksourceview.View(srcbuf)
      srcview.set_show_line_numbers(True)
      self.has_textview = True

    except:
      try:
        import gtksourceview
        srcbuf = gtksourceview.SourceBuffer()
        langman = gtksourceview.SourceLanguagesManager()
        lang = langman.get_language_from_mime_type("text/x-csrc")
        srcbuf.set_language(lang)
        srcbuf.set_highlight(True)
        srcview = gtksourceview.SourceView(srcbuf)
        srcview.set_show_line_numbers(True)
        self.has_textview = True

      except:
        pass

    if self.has_textview:
      sw = self.boxer.get_widget("srcview_scrolledwindow")
      sw.remove(self.textview)
      self.textview = srcview
      sw.add(self.textview)
      self.textbuffer = srcbuf
      sw.show_all()
      self.has_textview = True

    # try to set the default font
    try:
      from pango import FontDescription
      font = FontDescription(self.config.get("GUI", "font"))
      self.textview.modify_font(font)

    except:
      pass

    self.clipboard = gtk.Clipboard()

    # Find examples and populate the menu File->Examples
    self._fill_example_menu()

    # Set a template program to start with...
    if filename == None:
      self.raw_file_new()
    else:
      self.raw_file_open(filename)

    # Now set the focus on the text view
    self.textview.grab_focus()

  def _fill_example_menu(self):
    """Populate the example submenu File->Examples"""
    def create_callback(filename):
      def save_file(menuitem):
        if not self.ensure_file_is_saved(): return
        self.raw_file_open(filename)
      return save_file

    example_files = config.get_example_files()
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
  if config.use_threads:
    gtk.gdk.threads_init()
    gtk.gdk.threads_enter()

  main_window = Boxer(filename=filename, box_exec=box_exec)
  gtk.main()

  if config.use_threads:
    gtk.gdk.threads_leave()

