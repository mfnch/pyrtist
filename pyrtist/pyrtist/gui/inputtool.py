# Copyright (C) 2011
#  by Matteo Franchin (fnch@users.sourceforge.net)
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

from assistant import Action, Paste

import gtk

class InputTool(object):
  def __init__(self, parent=None, label="Input:"):
    self.window = w = gtk.Window(gtk.WINDOW_TOPLEVEL)
    self.border_frame = border_frame = gtk.Frame()

    self._value = None
    if parent != None:
      w.set_transient_for(parent)
      w.set_modal(True)

    border_frame.set_border_width(0)
    border_frame.set_shadow_type(gtk.SHADOW_IN)

    w.set_decorated(False)
    w.set_position(gtk.WIN_POS_MOUSE)
    w.add(border_frame)

    self.widget_vbox = vb = gtk.VBox()
    self.widget_label = l = gtk.Label(label)
    self.widget_entry = e = gtk.Entry()
    e.grab_focus()

    vb.pack_start(l, expand=False)
    vb.pack_start(e, expand=False)
    border_frame.add(vb)

    # Connect events
    self.window.connect("key-press-event", self.on_key_press)

    w.show_all()

  def quit(self):
    self.window.hide()
    gtk.main_quit()

  def on_key_press(self, widget, event):
    if event.keyval == gtk.keysyms.Return:
      self._value = self.widget_entry.get_text()
      self.quit()
      return False

    elif event.keyval == gtk.keysyms.Escape:
      self.quit()
      return False

    else:
      return None

  def get_value(self):
    """Get the final value entered by the user."""
    return self._value

  def set_value(self, v):
    """Set the default value."""
    self._value = v

  value = property(get_value, set_value)

  def run(self):
    self._value = None
    self.window.show()
    gtk.main()
    return self.value

class InputAct(Action):
  def __init__(self, text=None, string_input=False, **args):
    self.text = text if text != None else "$INPUT$"
    self.paste = Paste(text)
    self.inputtool_args = args
    self.inputtool = None
    self.string_input = string_input

  def execute(self, parent):
    if self.inputtool == None:
      self.inputtool = InputTool(parent.window, **self.inputtool_args)
    it = self.inputtool
    value = it.run()
    if value != None:
      if self.string_input:
        value = '"' + value.replace('"', '\\"') + '"'
      v = self.text.replace("$INPUT$", value)
      self.paste.text = v
      self.paste.execute(parent)


class FloatInputAct(InputAct):
  pass
