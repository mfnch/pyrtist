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


class CallAction(object):
  def __init__(self, fn):
    self.fn = fn

  def execute(self, parent):
    self.fn(parent)


def rinsert_char(tb, insert=", ", left=False, delims="),"):
  return insert_char(tb, insert=insert, left=left, delims=delims)

def insert_char(tb, insert=", ", left=True, delims="(,"):
  if tb is not None:
    it = tb.get_iter_at_mark(tb.get_insert())
    border_reached = it.starts_line if left else it.ends_line
    move_to_next = it.backward_char if left else it.forward_char
    while not border_reached() and move_to_next():
      c = it.get_char()
      if c in " \t":
        continue
      if c not in delims:
        tb.insert_at_cursor(insert)
      return

class Paste(Action):
  def __init__(self, text):
    self.text = text
    self.cursor_in = None
    self.cursor_out = None

  def execute(self, parent):
    text = self.text
    tb = parent.textbuffer

    if tb != None:
      tb.begin_user_action() # So that it appears on the Undo list

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
          insert_char(tb, delims="(,")

        elif tag == "RCOMMA":
          insert_char(tb, delims="),", left=False)

        elif tag == "LNEWLINE":
          insert_char(tb, delims="", insert="\n")

        elif tag == "RNEWLINE":
          insert_char(tb, delims="", insert="\n", left=False)

      if cursorin and not cursorin.get_deleted():
        it = tb.get_iter_at_mark(cursorin)
        tb.place_cursor(it)
        tb.delete_mark(cursorin)

      if cursorout:
        parent._set_exit_mark(tb, cursorout)

      tb.end_user_action()

      tv = parent.textview
      if tv != None:
        tv.grab_focus()


class GUISet(Action):
  def __init__(self, **values):
    self.values = values

  def execute(self, parent):
    gui = parent.gui
    if gui != None:
      gui.set_props(**self.values)


class GUIAct(Action):
  def __init__(self, *unnamed_values, **values):
    self.values = values
    for name in unnamed_values:
      values[name] = None

  def execute(self, parent):
    gui = parent.gui
    if gui != None:
      gui.do(**self.values)


class ExitMode(Action):
  def execute(self, parent):
    parent.exit_mode()

    tv = parent.textview
    if tv != None:
      tv.grab_focus()


def find_include(tb, includefile, startline=0):
  begin_it = tb.get_iter_at_line(startline)
  num_lines = tb.get_line_count()
  while startline < num_lines:
    startline += 1
    end_it = tb.get_iter_at_line(startline)
    line = tb.get_text(begin_it, end_it).strip().lower()
    if line.startswith("include"):
      if line[7:].strip().startswith('"%s"' % includefile):
        return True
    elif len(line) > 0 and not line.startswith("//"):
      return False
    begin_it = end_it


class IncludeSrc(Action):
  def __init__(self, include_filename):
    self.include_filename = include_filename

  def execute(self, parent):
    tb = parent.textbuffer
    if tb != None:
      it = tb.get_iter_at_line(0)
      if not find_include(tb, self.include_filename):
        tb.insert(it, 'include "%s"\n' % self.include_filename)


class Button(object):
  def __init__(self, name="", filename=None):
    self.filename = filename
    self.name = name

  def create_widget(self, **other_args):
    pass


def optlist(ol):
  if ol != None:
    return ol if hasattr(ol, "__iter__") and type(ol) != str else [ol]
    # ^^^ str has not __iter__ as attribute, but may in future versions...
  else:
    return []


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

  def execute_enter_actions(self, parent):
    """Call all the actions registered for the entrance of the mode."""
    self._execute_actions(self.enter_actions, parent)

  def execute_exit_actions(self, parent):
    """Call all the actions registered for the exit of the mode."""
    self._execute_actions(self.exit_actions, parent)

  def _execute_actions(self, actions, parent):
    if actions != None:
      for action in actions:
        action.execute(parent)

class Assistant(object):
  def __init__(self, main_mode):
    self.main_mode = main_mode
    self.start()
    self.permanent_modes = filter(lambda m: m.permanent, main_mode.submodes)
    self.window = None
    self.textview = None
    self.textbuffer = None
    self.statusbar = None
    self.gui = None

  def set_window(self, w):
    """Set the parent GUI window."""
    self.window = w

  def set_textbuffer(self, tb):
    """Set the textbuffer where to put the output of the selected modes."""
    self.textbuffer = tb

  def set_textview(self, tv):
    """Set the textbuffer where to put the output of the selected modes."""
    self.textview = tv

  def set_statusbar(self, sb):
    """Set the textbuffer where to put the output of the selected modes."""
    self.statusbar = sb

  def set_gui(self, gui):
    """Set the GUI which will be the target of the GUISet actions."""
    self.gui = gui

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

    except ValueError:
      raise ValueError("Mode not found (%s)" % new_mode)

    # Enter the mode only if it has any submodes
    if mode.submodes != None:
      self.current_modes.append(mode)
      self._exit_marks.append(None)

      if self.statusbar != None:
        context_id = self.statusbar.get_context_id(mode.name)
        msg = mode.statusbar_text
        msg = "%s: %s" % (mode.name, msg) if msg else mode.name + "."
        message_id = self.statusbar.push(context_id, msg)

    # Execute action for mode
    mode.execute_enter_actions(self)
    if mode.submodes == None:
      mode.execute_exit_actions(self)

  def exit_mode(self):
    """Exit the current mode (executing exit actions, if necessary)."""
    if len(self.current_modes) > 1:
      # Execute exit actions
      self.get_current_mode().execute_exit_actions(self)

      # Remove mode
      mode = self.current_modes.pop()
      item = self._exit_marks.pop()

      # Remove statusbar message
      if self.statusbar != None:
        context_id = self.statusbar.get_context_id(mode.name)
        self.statusbar.pop(context_id)

      if item != None:
        tb, cursorout = item
        if not cursorout.get_deleted():
          it = tb.get_iter_at_mark(cursorout)
          tb.place_cursor(it)
          tb.delete_mark(cursorout)

  def exit_all_modes(self, force=False):
    """Exit all opened modes. When 'force=True', the modes are exited in non
    interactive mode (this is ignored now, but may be used in future
    versions)."""
    while len(self.current_modes) > 1:
      self.exit_mode()

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
