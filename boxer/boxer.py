import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import os

import config

def box_source_preamble(out_file, out_img_file):
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

  """ % (out_file, out_img_file)

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

def tmp_file(base, ext=""):
  return "/tmp/boxer_%s.%s" % (base, ext)

class Boxer:
  def delete_event(self, widget, event, data=None):
    return False

  def raw_quit(self):
    """Called to quit the program."""
    gtk.main_quit()

  def destroy(self, widget, data=None):
    self.raw_quit()

  def map_coord_to_box(self, py_coord):
    """Map coordinates of the mouse on the GTK widget to Box coordinates."""

    if self.bbox == None:
      return None

    py_x, py_y = py_coord
    py_sx = self.outimage.allocation.width
    py_sy = self.outimage.allocation.height

    ((box_min_x, box_min_y), (box_max_x, box_max_y)) = self.bbox
    x = box_min_x + (box_max_x - box_min_x)*py_x/py_sx
    y = box_max_y + (box_min_y - box_max_y)*py_y/py_sy
    return (x, y)

  def map_coord_from_box(self, box_coord):
    """Map Box coordinates to on the GTK widget."""

    if self.bbox == None:
      return None

    ((box_min_x, box_min_y), (box_max_x, box_max_y)) = self.bbox
    box_x, box_y = box_coord
    py_sx = self.outimage.allocation.width
    py_sy = self.outimage.allocation.height
    x = py_sx*(box_x - box_min_x)/(box_max_x - box_min_x)
    y = py_sy*(box_y - box_min_y)/(box_max_y - box_min_y)
    return (x, y)

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
    if not_undoable: self.textbuffer.end_not_undoable_action()

  def raw_file_new(self):
    """Start a new box program and set the content of the main textview."""
    from config import box_source_of_new
    self.set_main_source(box_source_of_new)
    self.filename = None

  def raw_file_open(self, filename):
    """Load the file 'filename' into the textview."""
    try:
      f = open(filename, "r")
      self.set_main_source(f.read())
      f.close()
      self.filename = filename

    except:
      print "Error loading the file"

  def raw_file_save(self, filename=None):
    """Save the textview content into the file 'filename'."""
    try:
      if filename == None:
        filename = self.filename
      f = open(filename, "w")
      f.write(self.get_main_source())
      f.close()
      self.filename = filename

    except:
      print "Error saving the file"

  def menu_file_new(self, image_menu_item):
    self.raw_file_new()

  def menu_file_open(self, image_menu_item):
    """Ivoked to open a file. Shows the dialog to select the file."""
    fc = gtk.FileChooserDialog(title="Open Box program",
                               parent=self.mainwin,
                               action=gtk.FILE_CHOOSER_ACTION_OPEN,
                               buttons=("_Cancel", gtk.RESPONSE_CANCEL,
                                        "_Open", gtk.RESPONSE_OK))
    try:
      response = fc.run()
      if response == gtk.RESPONSE_OK:
        self.raw_file_open(fc.get_filename())
    except:
      pass

    fc.destroy()

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
    try:
      response = fc.run()
      if response == gtk.RESPONSE_OK:
        self.raw_file_save(fc.get_filename())
    except:
      pass

    fc.destroy()

  def menu_file_quit(self, image_menu_item):
    """Called on file->quit to quit the program."""
    self.raw_quit()

  def menu_edit_undo(self, image_menu_item):
    """Called on a CTRL+Z or menu->undo."""
    try:
      if self.textbuffer.can_undo():
        self.textbuffer.undo()
    except:
      print "Cannot undo :("

  def menu_edit_redo(self, image_menu_item):
    """Called on a CTRL+SHIFT+Z or menu->redo."""
    try:
      if self.textbuffer.can_redo():
        self.textbuffer.redo()
    except:
      print "Cannot redo :("

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
    """Called menu run->execute command."""
    tb = self.textbuffer
    box_source = tb.get_text(tb.get_start_iter(), tb.get_end_iter())
    box_source_file = tmp_file("source", "box")
    box_out_file = tmp_file("out", "dat")
    box_out_img_file = tmp_file("out", "png")
    preamble = box_source_preamble(box_out_file, box_out_img_file)
    f = open(box_source_file, "w")
    f.write(preamble + box_source)
    f.close()
    import commands

    for filename in [box_out_file, box_out_img_file]:
      try:
        os.unlink(filename)
      except:
        pass

    box_out_msgs = commands.getoutput("box -l g %s" % box_source_file)
    self.outtextbuffer.set_text(box_out_msgs)

    try:
      data = parse_out_file(box_out_file)
      bbox_n = int(data["bbox_n"])
      bbox_min = (float(data["bbox_min_x"]), float(data["bbox_min_y"]))
      bbox_max = (float(data["bbox_max_x"]), float(data["bbox_max_y"]))
      if bbox_n == 3:
        self.bbox = [bbox_min, bbox_max]
      else:
        self.bbox = None

    except:
      self.bbox = None

    if os.access(box_out_img_file, os.R_OK):
      self.outimage.set_from_file(box_out_img_file)

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

  def outimage_motion(self, eventbox, event):
    pass

  def outimage_click(self, eventbox, event):
    """Called when clicking with the mouse over the image."""
    box_coord = self.map_coord_to_box(event.get_coords())
    if box_coord != None:
      point_name = self.pointman.add(box_coord)
      self.outimage_update_points()
      x, y = box_coord
      self.textbuffer.insert_at_cursor("(%s, %s), " % (x, y))

  def outimage_update_points(self):
    """Draw the reference point markers over the image."""
    pass

  def __init__(self, gladefile="boxer.glade"):
    self.config = config.Config()

    self.gladefile = gladefile
    self.boxer = gtk.glade.XML(self.gladefile, "boxer")
    self.mainwin = self.boxer.get_widget("boxer")
    self.textview = self.boxer.get_widget("textview")
    self.textbuffer = self.textview.get_buffer()
    self.outtextview = self.boxer.get_widget("outtextview")
    self.outtextbuffer = self.outtextview.get_buffer()
    self.outimage = self.boxer.get_widget("outimage")
    self.bbox = None
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
           "on_outimage_motion": self.outimage_motion,
           "on_outimage_click": self.outimage_click}
    self.boxer.signal_autoconnect(dic)

    # Replace the TextView with a SourceView, if possible...
    self.has_textview = False
    try:
      import gtksourceview
      srcbuf = gtksourceview.SourceBuffer()
      langman = gtksourceview.SourceLanguagesManager()

      lang = langman.get_language_from_mime_type("text/x-csrc")
      #langS.set_mime_types(["text/x-python"])
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

    # Used to manage the reference points
    import pointman
    self.pointman = pointman.RefPointManager()

    self.clipboard = gtk.Clipboard()

    # Set a template program to start with...
    self.raw_file_new()

if __name__ == "__main__":
  form = Boxer()
  gtk.main()
