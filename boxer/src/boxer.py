# Copyright (C) 2008 by Matteo Franchin (fnch@users.sourceforge.net)
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

import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import gtk.gdk
import os
import os.path

import config
import document

def box_source_preamble(out_files):
  if out_files == None:
    return """
include "g"
GUI = Void
Window@GUI[]

"""

  else:
    box_out_file, box_out_img_file = out_files
    box_out_file = box_out_file.replace("\\", "\\\\")
    box_out_img_file = box_out_img_file.replace("\\", "\\\\")
    return """
include "g"
GUI = Void
Window@GUI[
  b = BBox[$]
  out = Str["bbox_n = ", b.n;
            "bbox_min_x = ", b.min.x; "bbox_min_y = ", b.min.y;
            "bbox_max_x = ", b.max.x; "bbox_max_y = ", b.max.y;]
  \ File["%s", "w"][out]
  $.Save["%s"]
]

""" % (box_out_file, box_out_img_file)

def parse_out_file(out_file):
  """Get the data parsing the output file produced by the GUI Box object."""
  known_vars = ["bbox_n", "bbox_min_x", "bbox_min_y",
                "bbox_max_x", "bbox_max_y"]
  f = open(out_file, "r")
  lines = f.read().splitlines()
  f.close()
  data = {}
  for line in lines:
    sides = line.split("=")
    if len(sides) == 2:
      lhs = sides[0].strip()
      rhs = sides[1].strip()
      if lhs in known_vars:
        data[lhs] = rhs
  return data

def debug():
  import sys
  from IPython.Shell import IPShellEmbed
  calling_frame = sys._getframe(1)
  IPShellEmbed([])(local_ns  = calling_frame.f_locals,
                   global_ns = calling_frame.f_globals)

_tmp_files = [[]]
def tmp_file(base, ext="", remember=True):
  try:
    import os
    prefix = "boxer%d%s" % (os.getpid(), base)
  except:
    prefix = "boxer%s" % base
  import tempfile
  filename = tempfile.mktemp(prefix=prefix, suffix="." + ext)
  if remember: _tmp_files[0].append(filename)
  return filename

def tmp_remove():
  """Remove all the temporary files created up to now."""
  for filename in _tmp_files[0]:
    try:
      os.unlink(filename)
    except:
      pass
  _tmp_files[0] = []

def exec_command(cmd, args=""):
  # This is not really how we should deal properly with this.
  # (We should avoid .read()). It is just a temporary solution.
  import os
  in_fd, out_fd, err_fd  = os.popen3("\"%s\" %s" % (cmd, args))
  content = out_fd.read() + err_fd.read()
  out_fd.close()
  err_fd.close()
  in_fd.close()
  return content

class Boxer:
  def delete_event(self, widget, event, data=None):
    return not self.ensure_file_is_saved()

  def raw_quit(self):
    """Called to quit the program."""
    gtk.main_quit()

  def destroy(self, widget, data=None):
    self.raw_quit()

  def assume_file_is_saved(self):
    """Called just after saving a file, to communicate that this has just
    happened.
    """
    self.textbuffer.set_modified(False)

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
    here_marker = self.config.get_default("src_marker_cursor_here")
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
    self.imgview.ref_point_del_all()
    self.set_main_source(box_source_of_new)
    self.filename = None
    self.assume_file_is_saved()

  def raw_file_open(self, filename):
    """Load the file 'filename' into the textview."""
    d = document.Document()
    try:
      d.load_from_file(filename)
    except:
      self.error("Error loading the file")
      return

    ref_points = d.get_refpoints()
    user_str = d.get_user_part()
    execute = False

    try:
      self.imgview.ref_point_del_all()
      self.imgview.add_from_list(ref_points)
      execute = True

    except:
      self.imgview.ref_point_del_all()
      user_str = src

    self.set_main_source(user_str)
    self.filename = filename
    self.assume_file_is_saved()

    if execute: self.menu_run_execute(None)

  def raw_file_save(self, filename=None):
    """Save the textview content into the file 'filename'."""
    try:
      if filename == None:
        filename = self.filename

      d = document.Document()
      d.new(refpoints=self.imgview.get_point_list(),
            code=self.get_main_source())
      d.save_to_file(filename)

      self.filename = filename
      self.assume_file_is_saved()
      return True

    except:
      self.error("Error saving the file")
      return False

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

  def menu_file_open(self, image_menu_item):
    """Ivoked to open a file. Shows the dialog to select the file."""
    if not self.ensure_file_is_saved(): return
    fc = gtk.FileChooserDialog(title="Open Box program",
                               parent=self.mainwin,
                               action=gtk.FILE_CHOOSER_ACTION_OPEN,
                               buttons=("_Cancel", gtk.RESPONSE_CANCEL,
                                        "_Open", gtk.RESPONSE_OK))

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

    response = fc.run()
    filename = None
    if response == gtk.RESPONSE_OK:
      filename = fc.get_filename()
    fc.destroy()
    if filename != None: self.raw_file_save(filename)

  def menu_file_quit(self, image_menu_item):
    """Called on file->quit to quit the program."""
    self.raw_quit()

  def menu_edit_undo(self, image_menu_item):
    """Called on a CTRL+Z or menu->undo."""
    try:
      if self.textbuffer.can_undo():
        self.textbuffer.undo()
    except:
      self.error("Cannot undo :(")

  def menu_edit_redo(self, image_menu_item):
    """Called on a CTRL+SHIFT+Z or menu->redo."""
    try:
      if self.textbuffer.can_redo():
        self.textbuffer.redo()
    except:
      self.error("Cannot redo :(")

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
    box_source_file = tmp_file("source", "box")
    box_presource_file = tmp_file("pre", "box")
    box_out_file = tmp_file("out", "dat")
    box_out_img_file = tmp_file("out", "png")

    d = document.Document()
    d.new(preamble=box_source_preamble((box_out_file, box_out_img_file)),
          refpoints=self.imgview.get_point_list(),
          code="")

    pre = d.save_to_str()
    src = self.get_main_source()

    f = open(box_presource_file, "wt")
    f.write(pre)
    f.close()

    f = open(box_source_file, "wt")
    f.write(src)
    f.close()

    for filename in [box_out_file, box_out_img_file]:
      try:
        os.unlink(filename)
      except:
        pass

    box_executable = self.config.box_executable()
    pre_path, pre_basename = os.path.split(box_presource_file)
    extra_opts = "-I ."
    if self.filename != None:
      p = os.path.split(self.filename)[0]
      if len(p) > 0: extra_opts = "-I %s" % p
    args = ("-l g -I %s %s -se %s %s"
            % (pre_path, extra_opts, pre_basename, box_source_file))
    box_out_msgs = exec_command(box_executable, args)
    self.outtextbuffer.set_text(box_out_msgs)
    self.outtextview_expander.set_expanded(len(box_out_msgs.strip()) > 0)

    bbox = None
    try:
      data = parse_out_file(box_out_file)
      bbox_n = int(data["bbox_n"])
      bbox_min = (float(data["bbox_min_x"]), float(data["bbox_min_y"]))
      bbox_max = (float(data["bbox_max_x"]), float(data["bbox_max_y"]))
      if bbox_n == 3:
        bbox = [bbox_min, bbox_max]

    except:
      pass

    if os.access(box_out_img_file, os.R_OK):
      try:
        self.imgview.set_from_file(box_out_img_file, bbox)
      except:
        pass

    tmp_remove()

  def menu_help_about(self, image_menu_item):
    """Called menu help->about command."""
    ad = gtk.AboutDialog()
    import info
    ad.set_name(info.name)
    ad.set_version(info.version)
    ad.set_comments(info.comment)
    ad.set_license(info.license)
    ad.set_authors(info.authors)
    ad.set_website(info.website)
    ad.set_logo(None)

    ad.run()
    ad.destroy()

  def imgview_click(self, eventbox, event):
    """Called when clicking with the mouse over the image."""
    py_coords = event.get_coords()
    picked = self.imgview.pick(py_coords)
    ref_point = None
    if picked != None:
      ref_point, _ = picked
      self.widget_refpoint_entry.set_text(ref_point.get_name())

    if event.button == self.button_left:
      if ref_point != None:
        self.dragging_ref_point = ref_point

      else:
        box_coords = self.imgview.map_coords_to_box(py_coords)
        if box_coords != None:
          point_name = self.imgview.ref_point_new(py_coords)
          if self.get_paste_on_new():
            self.textbuffer.insert_at_cursor("%s, " % point_name)

    elif self.dragging_ref_point != None:
      return

    elif event.button == self.button_center:
      if ref_point != None:
        self.textbuffer.insert_at_cursor("%s, " % ref_point.name)

    elif event.button == self.button_right:
      if ref_point != None:
        self.imgview.ref_point_del(ref_point.name)

  def imgview_motion(self, eventbox, event):
    if self.dragging_ref_point != None:
      name = self.dragging_ref_point.get_name()
      py_coords = event.get_coords()
      self.imgview.ref_point_move(name, py_coords)

  def imgview_release(self, eventbox, event):
    if self.dragging_ref_point != None:
      name = self.dragging_ref_point.get_name()
      py_coords = event.get_coords()
      self.imgview.ref_point_move(name, py_coords)
      self.dragging_ref_point = None
      self.menu_run_execute(None)

  def refpoint_entry_changed(self, _):
    self.refpoint_show_update()

  def notify_refpoint_new(self, name):
    liststore = self.widget_refpoint_box.get_model()
    liststore.insert(-1, row=(name,))

  def notify_refpoint_del(self, name):
    liststore = self.widget_refpoint_box.get_model()
    for item in liststore:
      if liststore.get_value(item.iter, 0) == name:
        liststore.remove(item.iter)
        return

  def refpoint_show_update(self):
    selection = self.widget_refpoint_entry.get_text()
    ratio = self.imgview.refpoint_get_visible_ratio(selection)
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
    self.imgview.refpoint_set_visible(selection, do_show)
    self.refpoint_show_update()

  def get_paste_on_new(self):
    """Return true if the name of the reference points should be pasted
    to the current edited source when they are created.
    """
    return self.pastenewbutton.get_active()

  def __init__(self, gladefile="boxer.glade", filename=None):
    self.config = config.Config()

    self.button_left = self.config.get_default("button_left")
    self.button_center = self.config.get_default("button_center")
    self.button_right = self.config.get_default("button_right")

    self.gladefile = config.glade_path(gladefile)
    self.boxer = gtk.glade.XML(self.gladefile, "boxer")
    self.mainwin = self.boxer.get_widget("boxer")
    self.textview = self.boxer.get_widget("textview")
    self.textbuffer = self.textview.get_buffer()
    self.outtextview = self.boxer.get_widget("outtextview")
    self.outtextbuffer = self.outtextview.get_buffer()
    self.outtextview_expander = self.boxer.get_widget("outtextview_expander")
    self.examplesmenu = self.boxer.get_widget("menu_file_examples")
    self.pastenewbutton = self.boxer.get_widget("toolbutton_pastenew")

    self.widget_refpoint_box = self.boxer.get_widget("refpoint_box")
    self.widget_refpoint_entry = self.boxer.get_widget("refpoint_entry")
    self.widget_refpoint_show = self.boxer.get_widget("refpoint_show")

    ref_point_size = self.config.get_default("ref_point_size")

    import gobject
    liststore = gtk.ListStore(gobject.TYPE_STRING)
    self.widget_refpoint_box.set_model(liststore)
    self.widget_refpoint_box.set_text_column(0)

    # Used to manage the reference points
    import imgview
    self.imgview = imgview.ImgView(self.boxer.get_widget("imgview"),
                                   ref_point_size)

    def set_next_refpoint(_, value):
      self.widget_refpoint_entry.set_text(value)
    def get_next_refpoint(_):
      return self.widget_refpoint_entry.get_text()
    self.imgview.set_attr("get_next_refpoint_name", get_next_refpoint)
    self.imgview.set_attr("set_next_refpoint_name", set_next_refpoint)
    self.imgview.set_attr("notify_refpoint_new", self.notify_refpoint_new)
    self.imgview.set_attr("notify_refpoint_del", self.notify_refpoint_del)

    set_next_refpoint(self.imgview, "gui1")

    self.dragging_ref_point = None

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
           "on_help_about_activate": self.menu_help_about,
           "on_toolbutton_new": self.menu_file_new,
           "on_toolbutton_open": self.menu_file_open,
           "on_toolbutton_save": self.menu_file_save,
           "on_toolbutton_run": self.menu_run_execute,
           "on_imgview_motion": self.imgview_motion,
           "on_imgview_click": self.imgview_click,
           "on_imgview_release": self.imgview_release,
           "on_refpoint_entry_changed": self.refpoint_entry_changed,
           "on_refpoint_show_clicked": self.refpoint_show_clicked}
    self.boxer.signal_autoconnect(dic)

    # Replace the TextView with a SourceView, if possible...
    self.has_textview = False
    try:
      import gtksourceview
      srcbuf = gtksourceview.SourceBuffer()
      langman = gtksourceview.SourceLanguagesManager()
      lang = langman.get_language_from_mime_type("text/x-csrc")
      srcbuf.set_language(lang)
      srcbuf.set_highlight(True)
      srcview = gtksourceview.SourceView(srcbuf)
      srcview.set_show_line_numbers(True)

      sw = self.boxer.get_widget("srcview_scrolledwindow")
      sw.remove(self.textview)
      self.textview = srcview
      sw.add(self.textview)
      self.textbuffer = srcbuf
      sw.show_all()
      self.has_textview = True

    except:
      pass

    # try to set the default font
    try:
      from pango import FontDescription
      font = FontDescription(self.config.get_default("font"))
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

    self.menu_run_execute(None)

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

def run(arg_list):
  filename = None
  if len(arg_list) > 1:
    filename = arg_list[1]
  main_window = Boxer(filename=filename)
  gtk.main()

if __name__ == "__main__":
  form = Boxer()
  gtk.main()
