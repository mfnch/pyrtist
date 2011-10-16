# Copyright (C) 2011 by Matteo Franchin (fnch@users.sf.net)
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

import gtk

import config

class BoxSrcView(object):
  def _init_textview(self):
    view = gtk.TextView()
    buf = view.get_buffer()
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
      return None

  def _init_gtksourceview2(self):
    try:
      import gtksourceview2 as gtksourceview
      srcbuf = gtksourceview.Buffer()
      langman = gtksourceview.LanguageManager()
      search_paths = langman.get_search_path()
      search_paths.append(config.get_hl_path())
      langman.set_search_path(search_paths)
      lang = langman.get_language("box")
      srcbuf.set_language(lang)
      srcbuf.set_highlight_syntax(True)
      srcview = gtksourceview.View(srcbuf)
      srcview.set_show_line_numbers(True)
      return (2, srcview, srcbuf)

    except:
      return None

  def __init__(self, use_gtksourceview=True, quickdoc=None):
    """Create a new sourceview using gtksourceview2 or gtksourceview,
    if they are available, otherwise return a TextView.
    """
    self.quickdoc = quickdoc

    first = (0 if use_gtksourceview else 2)
    init_fns = [self._init_gtksourceview2,
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

    view.set_wrap_mode(gtk.WRAP_WORD)
    view.set_has_tooltip(True)
    view.connect("query-tooltip", self._sighandler_query_tooltip)

  def _sighandler_query_tooltip(self, srcview, x, y, keyb_mode, tooltip, *etc):
    word = self.get_word_at_coords(x, y)
    if word != None:
      qd = self.quickdoc
      if qd != None:
        text = qd(word)
        if text != None and len(text) > 0:
          tooltip.set_text(text)
          return True

    else:
      return False

  def get_iter_at_coords(self, x, y):
    bx, by = self.view.window_to_buffer_coords(gtk.TEXT_WINDOW_WIDGET, x, y)
    return self.view.get_iter_at_location(bx, by)

  def get_word_at_coords(self, x, y, max_length=20):
    iter_begin = self.get_iter_at_coords(x, y)
    if iter_begin.get_char().isalnum():
      iter_end = iter_begin.copy()
      isnotalnum = lambda c, _: not c.isalnum()
      if iter_begin.backward_find_char(isnotalnum, None, None):
        if iter_begin.forward_char():
          if iter_end.forward_find_char(isnotalnum, (), None):
            return self.buf.get_text(iter_begin, iter_end)
    return None
