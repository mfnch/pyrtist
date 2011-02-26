# Copyright (C) 2011
#  by Matteo Franchin (fnch@users.sourceforge.net)
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

from assistant import Action, Paste

import gtk

known_box_colors = {
  0x000000: "color.black",
  0x0000ff: "color.red",
  0x00ff00: "color.green",
  0x00ffff: "color.yellow",
  0xff0000: "color.blue",
  0xff00ff: "color.magenta",
  0xffff00: "color.cyan",
  0xffffff: "color.white",
  0x00007f: "color.dark_red",
  0x007f00: "color.dark_green",
  0x007f7f: "color.dark_yellow",
  0x7f0000: "color.dark_blue",
  0x7f007f: "color.dark_magenta",
  0x7f7f00: "color.dark_cyan",
  0x7f7f7f: "color.grey"
}

def _cstr(color_component):
  return "%.3f" % (float(color_component)/0xffff)

def color_to_box_str(color, alpha=0):
  r = color.red
  g = color.green
  b = color.blue
  c32 = (r >> 8) + (g & 0xff00) + ((b & 0xff00) << 8)
  c = known_box_colors.get(c32, None)
  if c != None:
    return (c if alpha == 0xffff
            else "Color[%s, .a=%s]" % (c, _cstr(alpha)))

  else:
    if r == g == b:
      c = _cstr(r)
      return ("Color[%s]" % c if alpha == 0xffff
              else "Color[%s, .a=%s]" % (c, _cstr(alpha)))

    else:
      rgba = [_cstr(x) for x in (r, g, b)]
      if alpha != 0xffff:
        rgba.append(_cstr(alpha))
      return "Color[(%s)]" % (", ".join(rgba))


class ColorSelect(Action):
  def __init__(self, text=None):
    self.text = text if text != None else "$INPUT$"
    self.paste = Paste("")
    self.colordlg = None

  def execute(self, parent):
    if self.colordlg == None:
      self.colordlg = \
        gtk.ColorSelectionDialog("Select color")

    cd = self.colordlg
    colorsel = cd.colorsel
    colorsel.set_has_opacity_control(True)
    colorsel.set_has_palette(True)
    colorsel.set_previous_color(colorsel.get_current_color())

    response = cd.run()

    if response == gtk.RESPONSE_OK:
      color = colorsel.get_current_color()
      alpha = colorsel.get_current_alpha()
      color_string = color_to_box_str(color, alpha)
      self.paste.text = self.text.replace("$COLOR$", color_string)
      self.paste.execute(parent)

    cd.hide()

