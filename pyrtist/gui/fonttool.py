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

import os

import gtk

import config
from toolbox import ToolBox, Button
from assistant import Assistant, Mode, Action, CallAction, Paste

def create_font_modes(exit_action):
  default_fonts = [
    ("avg_b", "AvantGarde-Book"), ("avg_bo", "AvantGarde-BookOblique"),
    ("avg_d", "AvantGarde-Demi"), ("avg_do", "AvantGarde-DemiOblique"),

    ("cour", "Courier"), ("cour_o", "Courier-Oblique"),
    ("cour_b", "Courier-Bold"), ("cour_bo", "Courier-BoldOblique"),

    ("helv", "Helvetica"), ("helv_o", "Helvetica-Oblique"),
    ("helv_b", "Helvetica-Bold"), ("helv_bo", "Helvetica-BoldOblique"),

    ("helvn", "Helvetica-Narrow"), ("helvn_o", "Helvetica-Narrow-Oblique"),
    ("helvn_b", "Helvetica-Narrow-Bold"), ("helvn_bo", "Helvetica-Narrow-BoldOblique"),

    ("ncsch", "NewCenturySchlbk-Roman"), ("ncsch_i", "NewCenturySchlbk-Italic"),
    ("ncsch_b", "NewCenturySchlbk-Bold"), ("ncsch_bi", "NewCenturySchlbk-BoldItalic"),

    ("palat", "Palatino-Roman"), ("palat_i", "Palatino-Italic"),
    ("palat_b", "Palatino-Bold"), ("palat_bi", "Palatino-BoldItalic"),

    #("symbol", "Symbol"),

    ("times", "Times-Roman"), ("times_i", "Times-Italic"),
    ("times_b", "Times-Bold"), ("times_bi", "Times-BoldItalic"),

    ("zapfchan", "ZapfChancery-MediumItalic"),

    ("zapfding", "ZapfDingbats")]

  # Create the modes for all the default fonts (in the table above)
  font_modes = []
  for font_filename, font_name in default_fonts:
    full_filename = os.path.join("fonts", "%s.png" % font_filename)
    font_mode = \
      Mode(font_name,
           tooltip=font_name,
           button=Button(font_name, full_filename),
           enter_actions=[Paste("$LCOMMA$\"%s\"$RCOMMA$" % font_name),
                          exit_action])
    font_modes.append(font_mode)

  return font_modes


class FontTool(object):
  def __init__(self, parent=None, label="Input:"):
    self.window = w = gtk.Window(gtk.WINDOW_TOPLEVEL)
    self._value = None
    if parent != None:
      w.set_transient_for(parent)
      w.set_modal(True)

    w.set_decorated(False)
    w.set_position(gtk.WIN_POS_MOUSE)

    # Connect events
    self.window.connect("key-press-event", self.on_key_press)

    exit_action = CallAction(lambda _: self.quit())
    exit = \
      Mode("Exit",
           permanent=True,
           tooltip="Exit font selection",
           button=Button("Exit"),
           enter_actions=exit_action)

    font_modes = [exit] + create_font_modes(exit_action)
    main_mode = Mode("X", submodes=font_modes)

    self.assistant = a = Assistant(main_mode)
    self.toolbox = tb = ToolBox(a, columns=2, size_request=(-1, -1),
                                icon_path=config.icon_path())
    tb.show_all()
    w.add(tb)
    w.show()

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


class FontAct(Action):
  def __init__(self, text=None, string_input=False, **args):
    self.text = text if text != None else "$INPUT$"
    self.paste = Paste(text)
    self.inputtool_args = args
    self.inputtool = None
    self.string_input = string_input

  def execute(self, parent):
    if self.inputtool == None:
      self.inputtool = ft = FontTool(parent.window, **self.inputtool_args)
      ft.assistant.set_textview(parent.textview)
      ft.assistant.set_textbuffer(parent.textbuffer)

    it = self.inputtool
    value = it.run()
    if value != None:
      if self.string_input:
        value = '"' + value.replace('"', '\\"') + '"'
      v = self.text.replace("$INPUT$", value)
      self.paste.text = v
      self.paste.execute(parent)


