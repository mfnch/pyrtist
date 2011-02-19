# Copyright (C) 2011 by Matteo Franchin (fnch@users.sourceforge.net)
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

import re

from config import debug

_variable_re = re.compile("[$][^$]*[$]")

def get_str_tag_and_pos(s, pos=0):
  start = s.find("$", pos)
  if start != -1:
    end = s.find("$", start + 1)
    if end != -1:
      return (s[pos:start], s[start+1:end], end + 1)
  return (s[pos:], None, -1)


class Action(object):
  def execute(self, parent):
    pass


def insert_char(tb, left=True, delims="[,", insert=", "):
  if tb != None:
    it = tb.get_iter_at_mark(tb.get_insert())
    border_reached = it.starts_line if left else it.ends_line
    move_to_next = it.backward_char if left else it.forward_char
    while not border_reached():
      if move_to_next():
        c = it.get_char()
        if c in " \t":
          continue
        elif c not in delims:
          tb.insert_at_cursor(insert)
      return


class Paste(Action):
  def __init__(self, text):
    #self.substs = {}
    #self.tags = {}

    #def substitutor(var):
    #  var_name = var.group(0)
    #  self.tags[var_name] = (var.start(), var.end())
    #  return self.substs.get(var_name, "")

    self.text = text #re.sub(_variable_re, substitutor, text)
    self.cursor_in = None
    self.cursor_out = None

  def execute(self, parent):
    text = self.text
    tb = parent.textbuffer

    if tb != None:
      pos = 0
      cursorin = None
      cursorout = None
      while pos != -1:
        s, tag, pos = get_str_tag_and_pos(text, pos)
        if s != None:
          tb.insert_at_cursor(s)

        if tag == "CURSORIN":
          it = tb.get_iter_at_mark(tb.get_insert())
          cursorin = tb.create_mark(None, it, True)

        elif tag == "CURSOROUT":
          it = tb.get_iter_at_mark(tb.get_insert())
          cursorout = tb.create_mark(None, it, True)

        elif tag == "LCOMMA":
          insert_char(tb, delims="[,")

        elif tag == "RCOMMA":
          insert_char(tb, delims="],", left=False)

        elif tag == "LNEWLINE":
          insert_char(tb, delims="", insert="\n")

        elif tag == "RNEWLINE":
          insert_char(tb, delims="", insert="\n", left=False)

      if cursorin:
        it = tb.get_iter_at_mark(cursorin)
        tb.place_cursor(it)
        tb.delete_mark(cursorin)

      if cursorout:
        parent._set_exit_mark(tb, cursorout)


class Set(Action):
  def __init__(self, property_name, value):
    self.property_name = property_name
    self.value = value

class ExitMode(Action):
  def execute(self, parent):
    parent.exit_mode()
    parent.exit_mode()


class Button(object):
  def __init__(self, name="", filename=None):
    self.filename = filename
    self.name = name


def optlist(ol):
  return ol if hasattr(ol, "__iter__") and type(ol) != str else [ol]
  # ^^^ str has not __iter__ as attribute, but may in future versions...

class Mode(object):
  def __init__(self, name, button=None, enter_actions=None, exit_actions=None,
               permanent=False, submodes=None, tooltip=None, statusbar=None):
    self.name = name
    self.tooltip_text = tooltip
    self.statusbar_text = statusbar
    self.permanent = permanent
    self.button = button
    self.enter_actions = optlist(enter_actions)
    self.exit_actions = optlist(exit_actions)
    self.submodes = submodes

  def execute_actions(self, parent):
    for action in self.enter_actions:
      action.execute(parent)

class Assistant(object):
  def __init__(self, main_mode):
    self.main_mode = main_mode
    self.start()
    self.permanent_modes = filter(lambda m: m.permanent, main_mode.submodes)
    self.textview = None
    self.textbuffer = None
    self.statusbar = None

  def set_textbuffer(self, tb):
    """Set the textbuffer where to put the output of the selected modes."""
    self.textbuffer = tb

  def set_textview(self, tv):
    """Set the textbuffer where to put the output of the selected modes."""
    self.textview = tv

  def set_statusbar(self, sb):
    """Set the textbuffer where to put the output of the selected modes."""
    self.statusbar = sb

  def start(self):
    """Set (or reset) the mode to the main one."""
    self.current_modes = [self.main_mode]
    self._exit_marks = [None]

  def choose(self, new_mode):
    """Choose a submode from the ones available in the current mode."""
    modes = self.get_available_modes()
    mode_names = map(lambda m: m.name, modes)

    # Add mode to opened modes
    try:
        mode_idx = mode_names.index(new_mode)
        mode = modes[mode_idx]
        self.current_modes.append(mode)
        self._exit_marks.append(None)

    except ValueError:
      raise ValueError("Mode not found (%s)" % new_mode)

    if self.statusbar != None:
      context_id = self.statusbar.get_context_id(mode.name)
      msg = mode.statusbar_text
      msg = "%s: %s" % (mode.name, msg) if msg else mode.name + "."
      message_id = self.statusbar.push(context_id, msg)

    # Execute action for mode
    mode.execute_actions(self)

  def exit_mode(self):
    if len(self.current_modes) > 1:
      # Remove mode
      mode = self.current_modes.pop()

      # Remove statusbar message
      if self.statusbar != None:
        context_id = self.statusbar.get_context_id(mode.name)
        self.statusbar.pop(context_id)

      item = self._exit_marks.pop()
      if item != None:
        tb, cursorout = item
        it = tb.get_iter_at_mark(cursorout)
        tb.place_cursor(it)
        tb.delete_mark(cursorout)

  def _set_exit_mark(self, tb, mark):
    item = self._exit_marks[-1]
    if item != None:
      tb, cursorout = item
      tb.delete_mark(cursorout)
    self._exit_marks[-1] = (tb, mark)

  def get_available_modes(self):
    """Return the modes currently available."""
    if len(self.current_modes) > 0:
      current_mode = self.current_modes[-1]
      return (current_mode.submodes if current_mode.submodes != None else [])

    else:
      return []

  def get_available_mode_names(self):
    """Return the names of the modes currently available."""
    return map(lambda m: m.name, self.get_available_modes())

  def get_button_modes(self):
    """Return the buttons which should be currently displayed."""
    perm_modes = self.permanent_modes
    av_modes = self.get_available_modes()
    other_modes = filter(lambda m: m not in perm_modes, av_modes)
    return filter(lambda m: m.button != None, perm_modes + other_modes)

  def get_current_mode(self):
    """Return the mode currently selected."""
    return (self.current_modes[-1] if len(self.current_modes) > 0
            else self.main_mode)
