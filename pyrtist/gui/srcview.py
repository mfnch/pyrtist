# Copyright (C) 2011, 2020-2021, 2023 Matteo Franchin
#
# This file is part of Pyrtist.
#
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 2.1 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

import logging as L

import gi
from gi.repository import Gtk

from . import config
from .undoer import Undoer

# Functions for the undoer.
def delete_fn(_, srcview, del_offs, del_len):
  buf = srcview.buf
  del_start = buf.get_iter_at_offset(del_offs)
  del_end = buf.get_iter_at_offset(del_offs + del_len)
  buf.place_cursor(del_start)
  mark = buf.create_mark(None, del_start, False)
  buf.delete(del_start, del_end)
  srcview.view.scroll_to_mark(mark, 0.0, False, 0.5, 0.5)
  buf.delete_mark(mark)

def insert_fn(_, srcview, ins_text, ins_offs):
  buf = srcview.buf
  ins_start = buf.get_iter_at_offset(ins_offs)
  buf.place_cursor(ins_start)
  mark = buf.create_mark(None, ins_start, False)
  buf.insert(ins_start, ins_text)
  srcview.view.scroll_to_mark(mark, 0.0, False, 0.5, 0.5)
  buf.delete_mark(mark)


class SrcView(object):
  def _init_textview(self):
    view = Gtk.TextView()
    buf = view.get_buffer()
    L.warning('Using plain text view for source editing')
    return (0, view, buf)

  def _init_gtksourceview1(self):
    try:
      import gtksourceview
      srcbuf = gtksourceview.SourceBuffer()
      langman = gtksourceview.SourceLanguagesManager()
      lang = langman.get_language_from_mime_type("text/x-csrc")
      srcbuf.set_language(lang)
      srcbuf.set_highlight(True)
      srcview = gtksourceview.SourceView(srcbuf)
      srcview.set_show_line_numbers(True)
      return (1, srcview, srcbuf)

    except:
      L.warning('Cannot load gtksourceview 1')
      return None

  def _init_gtksourceview2(self):
    try:
      import gtksourceview2 as gtksourceview
      srcbuf = gtksourceview.Buffer()
      langman = gtksourceview.LanguageManager()
      search_paths = langman.get_search_path()
      search_paths.append(config.get_hl_path())
      langman.set_search_path(search_paths)
      lang = langman.get_language("python")
      srcbuf.set_language(lang)
      srcbuf.set_highlight_syntax(True)
      srcview = gtksourceview.View(srcbuf)
      srcview.set_show_line_numbers(True)
      srcview.set_auto_indent(True)
      return (2, srcview, srcbuf)

    except:
      L.warning('Cannot load gtksourceview 2')
      return None

  def _init_gtksourceview4(self):
    try:
      gi.require_version('GtkSource', '4')
      from gi.repository import GtkSource as gtksourceview
      srcbuf = gtksourceview.Buffer()
      langman = gtksourceview.LanguageManager()
      search_paths = langman.get_search_path()
      search_paths.append(config.get_hl_path())
      langman.set_search_path(search_paths)
      lang = langman.get_language("python")
      srcbuf.set_language(lang)
      srcbuf.set_highlight_syntax(True)
      srcview = gtksourceview.View.new_with_buffer(srcbuf)
      srcview.set_show_line_numbers(True)
      srcview.set_auto_indent(True)
      return (4, srcview, srcbuf)

    except:
      L.warning('Cannot load gtksourceview 4')
      return None

  def __init__(self, use_gtksourceview=True, quickdoc=None, undoer=None):
    """Create a new sourceview using gtksourceview2 or gtksourceview,
    if they are available, otherwise return a TextView.
    """
    self.quickdoc = quickdoc
    self.undoer = undoer or Undoer()

    first = (0 if use_gtksourceview else 2)
    init_fns = [self._init_gtksourceview4,
                self._init_gtksourceview2,
                self._init_gtksourceview1,
                self._init_textview]
    for attempted_mode in range(first, 3):
      mode_view_buf = init_fns[attempted_mode]()
      if mode_view_buf:
        break

    mode, view, buf = mode_view_buf
    self.mode = mode
    self.view = view
    self.buf = buf

    view.set_wrap_mode(Gtk.WrapMode.WORD)
    # TODO: re-enable this.
    #view.set_property("has-tooltip", True)
    #view.connect("query-tooltip", self._sighandler_query_tooltip)
    buf.connect("insert-text", self._sighandler_insert_text)
    buf.connect("delete-range", self._sighandler_delete_range)

  def _sighandler_query_tooltip(self, srcview, x, y, keyb_mode, tooltip, *etc):
    word = self.get_word_at_coords(x, y)
    if word is not None:
      qd = self.quickdoc
      if qd is not None:
        text = qd(word)
        if text is not None and len(text) > 0:
          tooltip.set_text(text)
          return True
    else:
      return False

  def _sighandler_insert_text(self, buf, text_iter, ins_text, ins_len):
    self.undoer.record_action(delete_fn, self, text_iter.get_offset(), ins_len)

  def get_text(self, iter_begin=None, iter_end=None, include_hidden_chars=True):
    if iter_begin is None:
      iter_begin = self.buf.get_start_iter()
    if iter_end is None:
      iter_end = self.buf.get_end_iter()
    return self.buf.get_text(iter_begin, iter_end, include_hidden_chars)

  def _sighandler_delete_range(self, buf, del_iter_start, del_iter_end):
    ins_text = self.get_text(del_iter_start, del_iter_end)
    self.undoer.record_action(insert_fn, self, ins_text,
                              del_iter_start.get_offset())

  def get_iter_at_coords(self, x, y):
    bx, by = self.view.window_to_buffer_coords(Gtk.TextWindowType.WIDGET, x, y)
    return self.view.get_iter_at_location(bx, by)

  def get_word_at_coords(self, x, y, max_length=20):
    iter_begin = self.get_iter_at_coords(x, y)
    if iter_begin.get_char().isalnum():
      iter_end = iter_begin.copy()
      isnotalnum = lambda c, _: not c.isalnum()
      if iter_begin.backward_find_char(isnotalnum, None, None):
        if iter_begin.forward_char():
          if iter_end.forward_find_char(isnotalnum, (), None):
            return self.get_text(iter_begin, iter_end)
    return None
