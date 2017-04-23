# Copyright (C) 2011 by Matteo Franchin (fnch@users.sourceforge.net)
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

import gobject
import gtk

import pango

import rstparser
from tree import DoxProc
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
    tags["title1"] = tb.create_tag("title2", weight=pango.WEIGHT_BOLD)
    tags["code"] = tb.create_tag("code", font="Courier", background="#dfdfff")

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
        #self.dox_on_move(myself, event)
        # ^^^ this is to just update the mouse pointer in case the function
        # self.dox_on_click_link changes the text view.

  def set_content(self, content, clear=True):
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

    if clear:
      tb.set_text("")

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


class DoxTable(gtk.Table):
  def __init__(self, rows=1, cols=2, on_click_link=None):
    gtk.Table.__init__(self)
    self.dox_cells = []
    self.dox_cur_cell = 0
    self.dox_on_click_link = on_click_link

  def _get_doxtextview(self):
    cells = self.dox_cells
    nr_cell = self.dox_cur_cell
    if nr_cell < len(cells):
      doxtextview = cells[nr_cell]
    else:
      doxtextview = DoxTextView(on_click_link=self.dox_on_click_link)
      cells.append(doxtextview)
    self.dox_cur_cell += 1
    return doxtextview

  def empty(self):
    '''Empty the table.'''
    nr_cells_inserted = self.dox_cur_cell
    for nr_cell in range(nr_cells_inserted):
      self.remove(self.dox_cells[nr_cell])
    self.dox_cur_cell = 0

  def populate(self, rows, title=None,
               xoptions=gtk.EXPAND|gtk.FILL,
               yoptions=gtk.FILL,
               xpadding=0, ypadding=0):
    nr_rows = len(rows)
    nr_cols = (len(rows[0]) if nr_rows > 0 else 1)

    # First, we remove the cells previously inserted
    self.empty()

    # Now we add the title, if necessary
    if nr_rows > 0:
      extra_rows = 0
      if title:
        extra_rows = 1
        doxtextview = self._get_doxtextview()
        doxtextview.set_content(title)
        self.attach(doxtextview, 0, nr_cols, 0, 1,
                    xoptions=xoptions, yoptions=yoptions,
                    xpadding=xpadding, ypadding=ypadding)

      # We finally, fill the table
      self.resize(nr_rows + extra_rows, 2)
      for nr_row_rel, row in enumerate(rows):
        nr_row = nr_row_rel + extra_rows
        for nr_col, col in enumerate(row):
          doxtextview = self._get_doxtextview()
          doxtextview.set_content(col)
          self.attach(doxtextview, nr_col, nr_col+1, nr_row, nr_row+1,
                      xoptions=xoptions, yoptions=yoptions,
                      xpadding=xpadding, ypadding=ypadding)

    self.show_all()


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

  def gen_comb_link(self, child_t, parent_t):
    return ("%s@%s" % (child_t, parent_t), "link")

  def gen_brief_intro(self, target, section="Intro"):
    brief = get_content_text(self.gen_target_section(target, section=section))
    return brief.rstrip().split(".", 1)[0]
    
  def gen_target_section(self, target, title=None,
                         section="Intro", newline="\n"):
    pieces = Writer.gen_target_section(self, target, section)
    if pieces != None and len(pieces) >= 1:
      first = pieces[0].lstrip()
      pieces[0] = (first[0].upper() + first[1:] if len(first) > 0 else first)
      if newline:
        pieces.append(newline)
      pieces = rstparser.parse(pieces)
      if title:
        pieces = [(title, "bold")] + pieces

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

  def gen_proc_table(self, t, title=None):
    children = t.children
    procs = self.tree.procs
    child_intro_list = []
    for child in children:
      proc_name = str(DoxProc(child, t))
      type_name = [self.gen_type_link(child)]
      intro = self.gen_target_section(procs.get(proc_name, None), newline="")
      if intro != None and len(intro) > 0:
        child_intro_list.append((type_name, intro))

    self.table.populate(child_intro_list, title=title)
    return []

  def gen_type_section(self, t, section_name, level=0):
    title = self.gen_type_section_title(t)
    body = self.gen_target_section(t)
    example = self.gen_target_section(t, section="Example", title="Example: ")

    # Generate the uses and used lines
    uses_line = self.gen_type_list("Uses: ", t.children)
    used_line = self.gen_type_list("Used by: ", t.parents)
    subtypes = self.gen_type_list("Subtypes: ", t.subtype_children)
    table_title = [("Uses the following types:", "title2")]
    proc_table = self.gen_proc_table(t, title=table_title)

    return (title + body + example + subtypes + used_line + uses_line
            + proc_table)

  def gen_instance_section(self, instance, level=0):
    '''Generate the help content for the given instance.'''
    title = self.gen_type_section_title(instance)
    body = self.gen_target_section(instance)
    example = self.gen_target_section(instance, section="Example", title="Example: ")
    self.table.empty()
    return title + body + example

  def gen_section_section(self, section, level=0):
    '''Generate the help content for the given section.'''
    return self.gen_instance_section(section, level=level)
