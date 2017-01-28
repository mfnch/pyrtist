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

import os
import types

import gtk

import config
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
    if img != None:
      b = gtk.Button()
      b.set_image(img)

    else:
      b = gtk.Button(self.name)

    if tooltip != None:
      b.set_tooltip_text(tooltip)
    return b


class HelpAct(assistant.Action):
  def __init__(self, help_text):
    self.help_text = help_text

  def execute(self, parent):
    md = gtk.MessageDialog(parent=parent.window,
                           flags=0,
                           type=gtk.MESSAGE_INFO,
                           buttons=gtk.BUTTONS_OK,
                           message_format=self.help_text)
    _ = md.run()
    md.destroy()


class ToolBox(gtk.Table):
  def __init__(self, assistant, columns=1, homogeneous=False,
               size_request=(-1, -1), config=None, icon_path=None):
    self.config = config
    self.icon_path = icon_path
    self.columns = columns
    self.assistant = assistant
    rows = self._compute_rows_from_cols()
    gtk.Table.__init__(self, rows=rows, columns=columns,
                       homogeneous=homogeneous)
    self._update_buttons()
    self.set_size_request(*size_request)

  def _get_button_size(self):
    '''Whether to use big buttons or small buttons.'''
    big_buttons = (self.config.getboolean("GUI", "big_buttons")
                   if self.config else False)
    return ((32, 32) if big_buttons else (24, 24))

  def _get_icon_path(self):
    '''Get the icon path for the particular icon size we use.'''
    if self.config:
      big_buttons = self.config.getboolean("GUI", "big_buttons")
      return config.icon_path(big_buttons=big_buttons)

    else:
      return self.icon_path

  def _compute_rows_from_cols(self):
    num_buttons = len(self.assistant.get_button_modes())
    return int((num_buttons + self.columns - 1)/self.columns)

  def _update_buttons(self):
    columns = self.columns
    rows = self._compute_rows_from_cols()
    button_modes = self.assistant.get_button_modes()
    width, height = self._get_button_size()
    icon_path = self._get_icon_path()

    for child in self.get_children():
      self.remove(child)

    # Populate the toolbox
    for i, button_mode in enumerate(button_modes):
      c = i % columns
      r = int(i / columns)

      button = button_mode.button
      button_widget = \
        button.create_widget(tooltip=button_mode.tooltip_text,
                             icon_path=icon_path,
                             width=width, height=height)

      button_widget.connect("clicked", self.button_clicked, button_mode)
      button_widget.show()
      self.attach(button_widget, c, c+1, r, r+1, xoptions=0, yoptions=0)

  def button_clicked(self, _, button_mode):
    """Called when a button in the toolbox is clicked."""
    self.assistant.choose(button_mode.name)
    self._update_buttons()

  def exit_all_modes(self, force=False):
    """Exit all the modes."""
    self.assistant.exit_all_modes(force=force)
    self._update_buttons()
