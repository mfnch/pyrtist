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

from assistant import Mode, ExitMode, Set, Paste, Button

exit = \
  Mode("Exit",
       permanent=True,
       tooltip="Exit the current mode",
       button=Button("Exit"),
       enter_actions=ExitMode())

color_red = Mode("Red",
                 button=Button("Red", "red.png"),
                 enter_actions=[Paste("$LCOMMA$color.red$CURSOROUT$$RCOMMA$"),
                                ExitMode()])

color_blue = Mode("Blue",
                  button=Button("Blue", "blue.png"),
                  enter_actions=[Paste("$LCOMMA$color.blue$CURSOROUT$$RCOMMA$"),
                                 ExitMode()])

color = \
  Mode("Color",
       tooltip="Select the color or create a new one",
       statusbar="Waiting for you to choose a color or create a new one",
       button=Button("Color", "color.png"),
       submodes=[color_red, color_blue, exit])

fill_void = Mode("Not-filled",
                 button=Button("Not filled", "fillvoid.png"),
                 enter_actions=[Paste("\"void\""), ExitMode()])

fill_plain = Mode("Plain",
                  button=Button("Plain", "fillplain.png"),
                  enter_actions=[Paste("\"plain\""), ExitMode()])

fill_eo = Mode("Even-Odd",
               button=Button("Even-Odd", "filleo.png"),
               enter_actions=[Paste("\"eo\""), ExitMode()])

fill = Mode("Fill mode",
            button=Button("Fill", "fill.png"),
            enter_actions=Paste("$LCOMMA$.Fill[$CURSORIN$]$CURSOROUT$$RCOMMA$"),
            submodes=[fill_void, fill_plain, fill_eo, exit])

border = Mode("Border",
              button=Button("Border", "border.png"),
              enter_actions=Paste("$LCOMMA$.Border[$CURSORIN$]$CURSOROUT$$RCOMMA$"),
              submodes=[color, exit])

style = Mode("Style",
             permanent=False,
             tooltip=("Choose the fill and border style for polygons, "
                      "circles, etc."),
             button=Button("Style", "style.png"),
             enter_actions=Paste("$LCOMMA$Style[$CURSORIN$]$CURSOROUT$$RCOMMA$"),
             submodes=[fill, border, exit])

poly = \
  Mode("Poly",
       tooltip="Create a polygon",
       statusbar=("Select the vertices by clicking on the image view; "
                  "choose the color and filling style from the toolbox"),
       button=Button("Poly", "poly.png"),
       enter_actions=[Paste("$LNEWLINE$\ .Poly[$CURSORIN$]$CURSOROUT$$RNEWLINE$"),
                      Set("PastePoint", True)],
       submodes=[color, style, exit])

circle = \
  Mode("Circle",
       tooltip="Create a new circle",
       button=Button("Circle", "circle.png"),
       enter_actions=[Paste("$LNEWLINE$\ .Circle[$CURSORIN$]$CURSOROUT$$RNEWLINE$"),
                      Set("PastePoint", True)],
       submodes=[color, style, exit])

line_sharp = \
  Mode("Sharp",
       button=Button("Sharp", "linesharp.png"),
       enter_actions=Paste("$LCOMMA$line.sharp$CURSOROUT$$RCOMMA$"))

line_smooth = \
  Mode("Smooth",
       button=Button("Smooth", "linesmooth.png"),
       enter_actions=Paste("$LCOMMA$line.medium$CURSOROUT$$RCOMMA$"))

line_vsmooth = \
  Mode("Very-Smooth",
       button=Button("V.Smooth", "linevsmooth.png"),
       enter_actions=Paste("$LCOMMA$line.smooth$CURSOROUT$$RCOMMA$"))

line_close = \
  Mode("Close",
       button=Button("Close", "lineclose.png"),
       enter_actions=Paste("$LCOMMA$.Close[]$CURSOROUT$$RCOMMA$"))

line = \
  Mode("Line",
       tooltip="Create a new line with fixed or variable thickness",
       button=Button("Line", "line.png"),
       enter_actions=[Paste("$LNEWLINE$\ .Line[$CURSORIN$]$CURSOROUT$$RNEWLINE$"),
                      Set("PastePoint", True)],
       submodes=[color, style, line_sharp, line_smooth, line_vsmooth,
                 line_close, exit])

font = \
  Mode("Font",
       tooltip="Choose the font (and size) used to render the text",
       button=Button("Font", "font.png"),
       enter_actions=[Paste("$LCOMMA$Font[$CURSORIN$]$CURSOROUT$$RCOMMA$")],
       submodes=[exit])

text = \
  Mode("Text",
       tooltip="Insert text into the figure",
       statusbar=("Select the font, enter the text, the position "
                  "and relative-positioning"),
       button=Button("Text", "text.png"),
       enter_actions=[Paste("$LNEWLINE$\ .Text[$CURSORIN$]$CURSOROUT$$RNEWLINE$"),
                      Set("PastePoint", True)],
       submodes=[font, color, style, exit])

main_mode = \
  Mode("Main", submodes=[exit, color, style, poly, circle, line, text])

if __name__ == "__main__":
  from assistant import Assistant
  assistant = Assistant(main_mode)
  assistant.start()

  while True:
    print assistant.get_available_mode_names()
    print "Select mode (or write esc):",
    new_mode = raw_input()
    if new_mode.strip().lower() == "esc":
      assistant.exit_mode()
    else:
      assistant.choose(new_mode)
