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

import os
import types

import gtk

import assistant

def build_icon_path(filename, icon_path=None):
  if icon_path == None:
    return filename

  elif type(icon_path) == types.FunctionType:
    return icon_path(filename)

  else:
    return os.path.join(icon_path, filename)


class Button(assistant.Button):
  def __init__(self, name="", filename=None):
    assistant.Button.__init__(self, name=name, filename=filename)

  def create_widget(self, tooltip=None, icon_path=None, **other_args):
    # Find the button image, if there is one
    img = None
    if self.filename:
      icon_filename = build_icon_path(self.filename, icon_path=icon_path)
      if os.path.exists(icon_filename):
        img = gtk.Image()
        img.set_from_file(icon_filename)

    # Create the button
    b = gtk.Button(self.name)
    if img != None:
      b.set_image(img)
      b.set_label("")

    if tooltip != None:
      b.set_tooltip_text(tooltip)
    return b


class ToolBox(gtk.Table):
  def __init__(self, assistant, columns=1, homogeneous=False,
               icon_path=None, size_request=(55, -1)):
    self.columns = columns
    self.assistant = assistant
    self.icon_path = icon_path
    rows = self._compute_rows_from_cols()
    gtk.Table.__init__(self, rows=rows, columns=columns,
                       homogeneous=homogeneous)
    self._update_buttons()
    self.set_size_request(*size_request)

  def _compute_rows_from_cols(self):
    num_buttons = len(self.assistant.get_button_modes())
    return int((num_buttons + self.columns - 1)/self.columns)

  def _update_buttons(self):
    columns = self.columns
    rows = self._compute_rows_from_cols()
    button_modes = self.assistant.get_button_modes()

    for child in self.get_children():
      self.remove(child)

    # Populate the toolbox
    for i, button_mode in enumerate(button_modes):
      c = i % columns
      r = int(i / columns)

      but = button_mode.button.create_widget(tooltip=button_mode.tooltip_text,
                                             icon_path=self.icon_path)
      but.connect("clicked", self.button_clicked, button_mode)
      but.show()
      self.attach(but, c, c+1, r, r+1, xoptions=0, yoptions=0)

  def button_clicked(self, _, button_mode):
    """Called when a button in the toolbox is clicked."""
    self.assistant.choose(button_mode.name)
    self._update_buttons()

  def exit_all_modes(self, force=False):
    """Exit all the modes."""
    self.assistant.exit_all_modes(force=force)
    self._update_buttons()
