# Copyright (C) 2011 by Matteo Franchin (fnch@users.sourceforge.net)
#
# This file is part of Box.
#
#   Box is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Box is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with Box.  If not, see <http://www.gnu.org/licenses/>.

import gobject
import gtk

import pango

from tree import DoxItem, DoxType, DoxProc, DoxTree
from writer import Writer


def join_list(l, sep):
  """Insert sep between consecutive elements of a list.
  Example:
    join_list([item1, item2, item3], sep)
      --> [item1, sep, item2, sep, item3]
  """
  n = len(l)
  if n < 2:
    return list(l)

  else:
    return [(l[i/2] if i % 2 == 0 else sep)
            for i in range(2*n - 1)]


def get_content_text(content):
  text = ""
  for piece in content:
    if piece != None:
      t_piece = type(piece)
      if t_piece == str:
        text += piece
      elif t_piece == gtk.TextChildAnchor:
        pass
      else:
        text += piece[0]
  return text


class DoxTextView(gtk.TextView):
  def __init__(self, on_click_link=None):
    gtk.TextView.__init__(self)
    self.set_editable(False)
    self.set_cursor_visible(False)
    self.set_wrap_mode(gtk.WRAP_WORD)
    self.set_pixels_above_lines(4)

    self.dox_on_click_link = on_click_link
    self.dox_cursor_normal = None
    self.dox_cursor_link = gtk.gdk.Cursor(gtk.gdk.HAND2)
    
    self.dox_tags = tags = {}
    tb = self.get_buffer()
    tags["bold"] = tb.create_tag("bold", weight=pango.WEIGHT_BOLD)
    tags["italic"] = tb.create_tag("italic", style=pango.STYLE_ITALIC)
    tags["link"] = tb.create_tag("link", underline=pango.UNDERLINE_SINGLE,
                                 foreground="#0000ff")
    tags["title1"] = tb.create_tag("title1", scale=pango.SCALE_LARGE,
                                   scale_set=True, weight=pango.WEIGHT_BOLD)
    tags["code"] = tb.create_tag("code") #, background="")

    self.connect("button-press-event", self.dox_on_click)
    self.connect("motion-notify-event", self.dox_on_move)
    self.connect("realize", self.dox_on_realize)

  def dox_on_realize(self, widget):
    w = self.get_window(gtk.TEXT_WINDOW_TEXT)
    w.set_cursor(self.dox_cursor_normal)

  def dox_get_iter_at_event(self, event):
    wx, wy = map(int, event.get_coords())
    bx, by = self.window_to_buffer_coords(gtk.TEXT_WINDOW_WIDGET, wx, wy)
    return self.get_iter_at_location(bx, by)

  def dox_is_over_link(self, event):
    it = self.dox_get_iter_at_event(event)
    return it.has_tag(self.dox_tags["link"])

  def dox_on_move(self, myself, event):
    w = self.get_window(gtk.TEXT_WINDOW_TEXT)
    w.set_cursor(self.dox_cursor_link
                 if self.dox_is_over_link(event)
                 else self.dox_cursor_normal)

  def dox_on_click(self, myself, event):
    it = self.dox_get_iter_at_event(event)
    link_tag = self.dox_tags["link"]
    if it.has_tag(link_tag):
      begin = it.copy()
      end = it.copy()
      while not begin.begins_tag(link_tag) and begin.backward_char(): pass
      while not end.ends_tag(link_tag) and end.forward_char(): pass
      link = self.get_buffer().get_text(begin, end)

      if callable(self.dox_on_click_link):
        self.dox_on_click_link(myself, link, event)
        self.dox_on_move(myself, event)
        # ^^^ this is to just update the mouse pointer in case the function
        # self.dox_on_click_link changes the text view.

  def set_content(self, content):
    """Output to textbuffer. output should be a list of items.
    Each item is either:
    - None (has no effect);
    - a string to be inserted at the current cursor position;
    - a tuple made of a string and and tags, in case the string should
      be inserted with tags.
    The function goes through the items and inserts them one after the other
    using the provided tags, if given.
    """
    tb = self.get_buffer()
    for piece in content:
      if piece != None:
        it = tb.get_iter_at_mark(tb.get_insert())
        t_piece = type(piece)
        if t_piece == str:
          tb.insert(it, piece)
        elif t_piece == gtk.TextChildAnchor:
          tb.insert_child_anchor(it, piece)
        else:
          tb.insert_with_tags_by_name(it, *piece)















class CellRendererDoxText(gtk.GenericCellRenderer):
  __gproperties__ = {"text": (gobject.TYPE_STRING, 'text to show',
                              'the text to show in the cell',
                              '', gobject.PARAM_READWRITE)}

  def __init__(self, **args):
    gtk.GenericCellRenderer.__init__(self)
    self.doxtextview = DoxTextView(**args)
    self.doxtextview.get_buffer().set_text("hello")

  def do_set_property(self, pspec, value):
    setattr(self, pspec.name, value)

  def do_get_property(self, pspec):
    return getattr(self, pspec.name)

  def on_get_size(self, widget, cell_area=None):
    return ((cell_area.x, cell_area.y, cell_area.width, cell_area.height)
            if cell_area != None else (0, 0, 0, 0))

  def on_render(self, window, widget, background_area, cell_area, expose_area, flags):
    widget.style.paint_layout(window, gtk.STATE_NORMAL, True,
                              cell_area, widget, 'footext',
                              cell_area.x, cell_area.y,
                              layout)
    print cell_area
    self.doxtextview.draw(cell_area) #window, widget, cell_area.x, cell_area.y)

  def get_text(self):
    tb = self.doxtextview.get_buffer()
    return tb.get_text()

  def set_text(self, text):
    print "Setting text"
    tb = self.doxtextview.get_buffer()
    tb.set_text(text)
  
  text = property(get_text, set_text)


gobject.type_register(CellRendererDoxText)


class DoxTable(object):
  def __init__(self):
    self.treestore = ts = gtk.TreeStore(str, str)
    self.treeview = tv = gtk.TreeView(ts)
    tvcol1 = gtk.TreeViewColumn("Type")
    tvcol2 = gtk.TreeViewColumn("Description")
    tv.append_column(tvcol1)
    tv.append_column(tvcol2)

    # create a CellRendererText to render the data
    cell1 = gtk.CellRendererText()
    cell2 = CellRendererDoxText()
    #cell2.set_property("wrap-width", 200)
    tvcol1.pack_start(cell1, True)
    tvcol2.pack_start(cell2, True)

    # set the cell "text" attribute to column 0 - retrieve text
    # from that column in treestore
    tvcol1.add_attribute(cell1, 'text', 0)
    tvcol2.add_attribute(cell2, 'text', 1)

    tv.set_search_column(0)
    tvcol1.set_sort_column_id(0)
    tv.set_reorderable(True)

  def populate(self, rows):
    ts = self.treestore
    ts.clear()
    for nr_row, row in enumerate(rows):
      ts.append(None, row)


class GtkWriter(Writer):
  def __init__(self, tree, textbuffer, table, docinfo={}, nest_subtypes=True):
    Writer.__init__(self, tree, docinfo=docinfo)
    self.nest_subtypes = nest_subtypes
    self.type_name_map = {}
    self.textbuffer = textbuffer
    self.table = table
    self.table_in_text = False
    self.anchor = None

  def gen_type_link(self, t):
    return (str(t), "link")
  
  def gen_target_section(self, target, section="Intro"):
    pieces = Writer.gen_target_section(self, target, section)
    if pieces != None and len(pieces) >= 1:
      first = pieces[0].lstrip()
      pieces[0] = (first[0].upper() + first[1:] if len(first) > 0 else first)
      pieces[-1] = pieces[-1].rstrip() + "\n"
    return pieces or []

  def gen_type_section_title(self, t, level=0):
    return [(str(t), "title1"), "\n"]

  def gen_type_list(self, name, type_list):
    ts = map(str, type_list)
    ls = []
    if len(ts) > 0:
      ts.sort()
      links = map(self.gen_type_link, ts)
      ls = [(name, "bold")] + join_list(links, ", ")
      ls.append("\n")
    return ls

  def gen_proc_table(self, t):
    children = t.children
    procs = self.tree.procs
    child_intro_list = []
    for child in children:
      proc_name = str(DoxProc(child, t))
      intro = self.gen_target_section(procs.get(proc_name, None))
      if intro != None and len(intro) > 0:
        child_intro_list.append((child, get_content_text(intro).strip()))

    self.table.populate(child_intro_list)
    return []

  def gen_type_section(self, t, section_name, level=0):
    title = self.gen_type_section_title(t)
    body = self.gen_target_section(t)

    # Generate the uses and used lines
    uses_line = self.gen_type_list("Uses: ", t.children)
    used_line = self.gen_type_list("Used by: ", t.parents)
    subtypes = self.gen_type_list("Subtypes: ", t.subtype_children)
    proc_table = self.gen_proc_table(t)

    return title + body + subtypes + used_line + uses_line + proc_table

  