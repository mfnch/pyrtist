# Copyright (C) 2013 by Matteo Franchin (fnch@users.sf.net)
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
#   along with Boxer. If not, see <http://www.gnu.org/licenses/>.

(DO_MODE_NORMAL,
 DO_MODE_UNDO,
 DO_MODE_REDO) = range(3)


class Undoer(object):
  def __init__(self):
    self.do_mode = DO_MODE_NORMAL
    self.not_undoable_count = 0
    self.undo_actions = []
    self.redo_actions = []
    self.group = []
    self.group_level = 0

  def register_undoable_action(self, name, do, undo):
    """Register a type of undoable action."""
    self.do_actions[name] = do
    self.undo_actions[name] = undo

  def clear(self):
    """Clear the undo/redo history."""
    self.undo_actions = []
    self.redo_actions = []

  def begin_not_undoable_action(self):
    """Begin a not undoable action. The undoer is cleared and deactivated.
    It is reactivated when end_not_undoable_action() is called as many times
    as begin_not_undoable_action() was."""
    if self.not_undoable_count == 0:
      self.clear()
    self.not_undoable_count += 1

  def end_not_undoable_action(self):
    """Marks the end of a not undoable action. A call to this function should
    match in pair with calls to begin_not_undoable_action()"""
    assert self.not_undoable_count > 0
    self.not_undoable_count -= 1

  def begin_group(self):
    """Begin a group of actions. A group of actions is seen as a single
    undoable action."""
    self.group_level += 1

  def end_group(self):
    """End a group of actions."""
    assert self.group_level > 0
    self.group_level -= 1
    if self.group_level == 0:
      def undo_group(_, group):
        self.begin_group()
        for action in group:
          action[0](action)
        self.end_group()
      self.record_action(undo_group, self.group)
      self.group = []

  def _do(self, action_list):
    if len(action_list) == 0:
      return
    action = action_list.pop()
    action[0](*action)

  def undo(self):
    """Undo the last action."""
    assert self.group_level == 0
    self.do_mode = DO_MODE_UNDO
    self._do(self.undo_actions)
    self.do_mode = DO_MODE_NORMAL

  def redo(self):
    """Redo the last undone action."""
    assert self.group_level == 0
    self.do_mode = DO_MODE_REDO
    self._do(self.redo_actions)
    self.do_mode = DO_MODE_NORMAL

  def record_action(self, *args):
    """Communicate to the Undoer that an action has been performed and provide
    means to undo that actions. In particular, it is assumed that if
    ``undoer.record_action(args)'' is called to signal an action ``x'', then
    ``args[0](args)'' undoes ``x''. It is also assumee that the function
    ``args[0]'' will call ``record_action'' to record how to undo the undone
    action."""
    if self.not_undoable_count != 0:
      return

    if self.group_level > 0:
      self.group.append(args)
      return

    do_mode = self.do_mode
    if do_mode == DO_MODE_REDO:
      self.undo_actions.append(args)
    elif do_mode == DO_MODE_UNDO:
      self.redo_actions.append(args)
    else:
      self.redo_actions = []
      self.undo_actions.append(args)
