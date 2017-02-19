# Copyright (C) 2013 by Matteo Franchin (fnch@users.sf.net)
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

(DO_MODE_NORMAL,
 DO_MODE_UNDO,
 DO_MODE_REDO) = range(3)


class UndoableGroup(object):
  def __init__(self, undoer):
    self._undoer = undoer

  def __enter__(self):
    self._undoer.begin_group()
    return self._undoer

  def __exit__(self, exc_type, exc_value, exc_traceback):
    self._undoer.end_group()


class Undoer(object):
  def __init__(self):
    self.do_mode = DO_MODE_NORMAL
    self.not_undoable_count = 0
    self.save_point = None
    self.undo_actions = []
    self.redo_actions = []
    self.group_actions = []
    self.group_level = 0
    self.callbacks = {"modified-changed": [],
                      "can-undo-changed": [],
                      "can-redo-changed": []}

  def register_callback(self, name, cb):
    """Register a type of undoable action."""
    if name == "all":
      keys = self.callbacks.keys()
      for key in keys:
        self.callbacks[key].append(cb)
    else:
      cb_list = self.callbacks.get(name, None)
      assert cb_list is not None, "Cannot find callback name %s" % name
      cb_list.append(cb)

  def _call_back(self, name, args):
    for cb in self.callbacks[name]:
      cb(self, name, args)

  def _begin_action(self):
    return (self.can_undo(), self.can_redo(), self.is_modified())

  def _end_action(self, old_values):
    new_values = self._begin_action()
    for idx, cb_name in enumerate(("can-undo", "can-redo", "modified")):
      new_value = new_values[idx]
      if new_value != old_values[idx]:
        self._call_back(cb_name + "-changed", new_value)

  def clear(self, mark_as_unmodified=True):
    """Clear the undo/redo history."""
    values = self._begin_action()
    self.undo_actions = []
    self.redo_actions = []
    if mark_as_unmodified:
      self.mark_as_unmodified()
    self._end_action(values)

  def mark_as_unmodified(self):
    """Mark this as a save point."""
    assert self.group_level == 0, \
      "Cannot mark as unmodified while grouping actions"
    values = self._begin_action()
    self.save_point = len(self.undo_actions)
    self._end_action(values)

  def is_modified(self):
    """Return True if and only if there have not been any modifications since
    the last call to mark_as_unmodified(). This means that either there were no
    recorded changes or that all the recorded/undone changes were
    undone/redone."""
    return self.group_level != 0 or len(self.undo_actions) != self.save_point

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
    if self.group_level == 0 and len(self.group_actions) > 0:
      def undo_group(_, group):
        self.begin_group()
        for action in reversed(group):
          action[0](*action)
        self.end_group()
      self.record_action(undo_group, self.group_actions)
      self.group_actions = []

  def group(self):
    """Return a context manager which can be used in the `with` statement to
    automatically call Undoer.begin_group() and Undoer.end_group().
    """
    return UndoableGroup(self)

  def can_undo(self):
    """Whether there are actions to undo."""
    return len(self.undo_actions) != 0

  def can_redo(self):
    """Whether there are actions to redo."""
    return len(self.redo_actions) != 0

  def _do(self, action_list, do_mode=DO_MODE_NORMAL):
    assert self.group_level == 0
    if len(action_list) == 0:
      return
    values = self._begin_action()
    action = action_list.pop()
    self.do_mode = do_mode
    action[0](*action)
    self.do_mode = DO_MODE_NORMAL
    self._end_action(values)

  def undo(self):
    """Undo the last action."""
    self._do(self.undo_actions, do_mode=DO_MODE_UNDO)

  def redo(self):
    """Redo the last undone action."""
    self._do(self.redo_actions, do_mode=DO_MODE_REDO)

  def record_action(self, *args):
    """Communicate to the Undoer that an action has been performed and provide
    means to undo that actions. In particular, it is assumed that if
    ``undoer.record_action(args)'' is called to signal an action ``x'', then
    ``args[0](args)'' undoes ``x''. It is also assumed that the function
    ``args[0]'' calls ``record_action'' to record how to undo the undone
    action."""
    if self.not_undoable_count != 0:
      self.save_point = None
      return

    if self.group_level > 0:
      self.group_actions.append(args)
      return

    # If one of these changes, then a callback is called back.
    values = self._begin_action()

    do_mode = self.do_mode
    if do_mode == DO_MODE_REDO:
      self.undo_actions.append(args)
    elif do_mode == DO_MODE_UNDO:
      self.redo_actions.append(args)
    else:
      if (self.save_point is not None and
          self.save_point > len(self.undo_actions)):
        self.save_point = None
      self.redo_actions = []
      self.undo_actions.append(args)

    # Generate callbacks
    self._end_action(values)
