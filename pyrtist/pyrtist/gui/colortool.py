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

from assistant import Action, Paste, Button

import gtk
import gtk.gdk

known_box_colors = {
  0x000000: "Color.black",
  0xff0000: "Color.red",
  0x00ff00: "Color.green",
  0xffff00: "Color.yellow",
  0x0000ff: "Color.blue",
  0xff00ff: "Color.magenta",
  0x00ffff: "Color.cyan",
  0xffffff: "Color.white",
  0x7f0000: "Color.dark_red",
  0x007f00: "Color.dark_green",
  0x7f7f00: "Color.dark_yellow",
  0x00007f: "Color.dark_blue",
  0x7f007f: "Color.dark_magenta",
  0x007f7f: "Color.dark_cyan",
  0x7f7f7f: "Color.grey"
}

def _cstr(cc):
  return "%.3f" % cc

class MyColor(object):
  def __init__(self, *components, **named_args):
    max_color = 1.0
    for key in named_args:
      if key == "max_color":
        max_color = named_args[key]
      else:
        raise TypeError("Unexpected keyword argument '%s'" % key)

    l = len(components)
    if l == 3:
      r, g, b = map(lambda x: float(x)/max_color, components)
      a = 1.0

    elif l == 4:
      r, g, b, a = map(lambda x: float(x)/max_color, components)

    else:
      raise ValueError("Bad usage of MyColor: you tried MyColor%s" % str(components))

    self.red = r
    self.green = g
    self.blue = b
    self.alpha = a
    self.adjust()

  def __eq__(self, other_color):
    return (isinstance(other_color, MyColor)
            and (self.red == other_color.red
                 and self.green == other_color.green
                 and self.blue == other_color.blue
                 and self.alpha == other_color.alpha))

  def __iter__(self):
    yield self.red
    yield self.green
    yield self.blue
    yield self.alpha

  def __int__(self):
    rgba = tuple(self)
    rgba_int = map(lambda x: int(max(0.0, min(255.0, round(x*255.0)))), rgba)
    return reduce(lambda sofar, x: (sofar << 8) | x, rgba_int)

  def adjust(self):
    rgba = map(lambda x: max(0.0, min(1.0, x)), tuple(self))
    self.red, self.green, self.blue, self.alpha = rgba

  def to_box_source(self):
    r, g, b, alpha = tuple(self)
    c = known_box_colors.get((int(self) & 0xffffff00) >> 8, None)
    if alpha >= 1.0 and c is not None:
      return c

    if r == g == b:
      c = _cstr(r)
      return ("Grey({})".format(c) if alpha >= 1.0
              else "Grey({}, {})".format(c, _cstr(alpha)))

    rgba = [_cstr(x) for x in (r, g, b)]
    if alpha < 1.0:
      rgba.append(_cstr(alpha))
    return "Color({})".format(", ".join(rgba))


class ColorSelect(Action):
  def __init__(self, text=None, history=None):
    self.text = text if text is not None else "$INPUT$"
    self.paste = Paste("")
    self.colordlg = None
    self.history = history

  def execute(self, parent):
    if self.colordlg is None:
      self.colordlg = \
        gtk.ColorSelectionDialog("Select color")

    cd = self.colordlg
    colorsel = cd.colorsel
    colorsel.set_has_opacity_control(True)
    colorsel.set_has_palette(True)
    colorsel.set_previous_color(colorsel.get_current_color())

    response = cd.run()

    if response == gtk.RESPONSE_OK:
      c = colorsel.get_current_color()
      alpha = colorsel.get_current_alpha()
      my_c = MyColor(c.red, c.green, c.blue, alpha, max_color=0xffff)
      color_string = my_c.to_box_source()
      self.paste.text = self.text.replace("$COLOR$", color_string)
      self.paste.execute(parent)
      if self.history is not None:
        self.history.new_color(my_c)

    cd.hide()


class ColorHistoryPaste(Action):
  def __init__(self, history, index, text):
    self.history = history
    self.index = index
    self.text = text if text is not None else "$COLOR$"
    self.paste = Paste("")

  def execute(self, parent):
    color = self.history.get_color(self.index)
    color_string = color.to_box_source()
    self.paste.text = self.text.replace("$COLOR$", color_string)
    self.paste.execute(parent)
    self.history.new_color(color)


class ColorHistoryButton(Button):
  def __init__(self, history, index):
    Button.__init__(self, str(index))
    self.history = history
    self.index = index

  def create_widget(self, tooltip=None, width=24, height=24, **other_args):
    # Find the button image, if there is one
    img = None

    # Create the button
    b = gtk.Button()
    my_color = self.history.get_color(self.index)
    if my_color is not None:
      pb = gtk.gdk.Pixbuf(gtk.gdk.COLORSPACE_RGB, False, 8, width, height)
      pb.fill(int(my_color))
      img = gtk.Image()
      img.set_from_pixbuf(pb)
      b.set_image(img)

    if tooltip is None:
      tooltip = "Recent color # $INDEX$: $COLOR$"

    if len(tooltip) > 0:
      t = tooltip.replace("$INDEX$", str(self.index))
      t = t.replace("$COLOR$", my_color.to_box_source())
      b.set_tooltip_text(t)
    return b


class ColorHistory(object):
  def __init__(self, length=10):
    self.length = length
    self.history = [None]*length
    self.index = 0

  def new_ColorSelect(self, **args):
    """Return a new ColorSelect action bound to this ColorHistory."""
    return ColorSelect(history=self, **args)

  def new_color(self, *colors):
    """Notify the usage of one or more new colors."""
    for color in colors:
      c = MyColor(*color)
      try:
        idx = self.history.index(c)
        item = self.history.pop(idx)
        if idx < self.index:
          self.index = (self.index - 1) % self.length
        self.history.insert(self.index, item)
        assert len(self.history) == self.length

      except ValueError:
        self.index = (self.index - 1) % self.length
        self.history[self.index] = c

  def get_color(self, history_index):
    assert history_index < self.length
    return self.history[(self.index + history_index) % self.length]

  def color_button(self, history_index, default_color=None):
    return ColorHistoryButton(self, history_index)

  def paste(self, history_index, paste_str):
    return ColorHistoryPaste(self, history_index, paste_str)
