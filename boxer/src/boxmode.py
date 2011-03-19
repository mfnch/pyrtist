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

from assistant \
  import Mode, ExitMode, GUISet, GUIAct, Paste, Button, IncludeSrc
from inputtool import InputAct
from colortool import ColorSelect


def shorten_string(s, max_length=5):
  return "%s..." % s[:3] if len(s) > max_length else s

# Commonly used actions
exit_action = ExitMode()
update_now = GUIAct("update")
push_settings = GUIAct("push_settings")
pop_settings = GUIAct("pop_settings")
update_on_paste = GUISet(update_on_paste=True)
paste_on_new = GUISet(paste_point=True)
dont_paste_on_new = GUISet(paste_point=False)

exit = \
  Mode("Exit",
       permanent=True,
       tooltip="Exit the current mode",
       button=Button("Exit"),
       enter_actions=exit_action)

color_new = Mode("Color New",
                 button=Button("New", "colornew.png"),
                 enter_actions=[ColorSelect("$LCOMMA$$COLOR$$RCOMMA$"),
                                update_now, exit_action])

color_black = Mode("Black",
                   button=Button("Black", "black.png"),
                   enter_actions=[Paste("$LCOMMA$color.black$RCOMMA$"),
                                  update_now, exit_action])

color_red = Mode("Red",
                 button=Button("Red", "red.png"),
                 enter_actions=[Paste("$LCOMMA$color.red$RCOMMA$"),
                                update_now, exit_action])

color_blue = Mode("Blue",
                  button=Button("Blue", "blue.png"),
                  enter_actions=[Paste("$LCOMMA$color.blue$RCOMMA$"),
                                 update_now, exit_action])

color = \
  Mode("Color",
       tooltip="Select the color or create a new one",
       statusbar="Waiting for you to choose a color or create a new one",
       button=Button("Color", "color.png"),
       submodes=[color_new, color_black, color_red, color_blue, exit])

gradient_line = \
  Mode("Gradient.Line",
       tooltip="Select linear gradient",
       statusbar="Enter initial and final point",
       button=Button("Linear", "lingrad.png"),
       enter_actions=[Paste("$LCOMMA$.Line[$CURSORIN$]$CURSOROUT$$RCOMMA$"),
                      push_settings, paste_on_new],
       exit_actions=pop_settings,
       submodes=[exit])

gradient_circle = \
  Mode("Gradient.Circle",
       tooltip="Select radial gradient",
       statusbar="Expecting: circle_centre_1, radius_1; circle_centre_2, radius_2",
       button=Button("Circular", "circgrad.png"),
       enter_actions=[Paste("$LCOMMA$.Circle[$CURSORIN$]$CURSOROUT$$RCOMMA$"),
                      push_settings, paste_on_new],
       exit_actions=pop_settings,
       submodes=[exit])

gradient = \
  Mode("Gradient",
       tooltip="Create a color gradient",
       statusbar="Waiting for you to choose between linear and radial gradient",
       button=Button("Gradient", "gradient.png"),
       enter_actions=[Paste("$LCOMMA$Gradient[$CURSORIN$]$CURSOROUT$$RCOMMA$"),
                      push_settings, dont_paste_on_new],
       exit_actions=pop_settings,
       submodes=[gradient_line, gradient_circle, color, exit])

fill_void = Mode("Not-filled",
                 button=Button("Not filled", "fillvoid.png"),
                 enter_actions=[Paste("\"void\""), exit_action])

fill_plain = Mode("Plain",
                  button=Button("Plain", "fillplain.png"),
                  enter_actions=[Paste("\"plain\""), exit_action])

fill_eo = Mode("Even-Odd",
               button=Button("Even-Odd", "filleo.png"),
               enter_actions=[Paste("\"eo\""), exit_action])

fill = Mode("Fill mode",
            button=Button("Fill", "fill.png"),
            enter_actions=Paste("$LCOMMA$.Fill[$CURSORIN$]$CURSOROUT$$RCOMMA$"),
            submodes=[fill_void, fill_plain, fill_eo, exit])

border_width = \
  Mode("Border width",
       button=Button("Width", "borderwidth.png"),
       enter_actions=[InputAct("$LCOMMA$$INPUT$$RCOMMA$",
                               label="Width of border:")])

border = Mode("Border",
              button=Button("Border", "border.png"),
              enter_actions=Paste("$LCOMMA$.Border[$CURSORIN$]$CURSOROUT$$RCOMMA$"),
              submodes=[color, border_width, exit])

style = \
  Mode("Style",
       permanent=False,
       tooltip=("Choose the fill and border style for polygons, "
                "circles, etc."),
       button=Button("Style", "style.png"),
       enter_actions=Paste("$LCOMMA$Style[$CURSORIN$]$CURSOROUT$$RCOMMA$"),
       submodes=[fill, border, exit])

poly_smoothing = \
  Mode("Poly smoothing",
       tooltip="Smooth the corners of the polygon",
       button=Button("Corners", "polysmooth.png"),
       enter_actions=[InputAct("$LCOMMA$$INPUT$$RCOMMA$",
                               label=("Smoothing of corners\n"
                                      "(between 0.0 and 1.0):"))])

poly_close = \
  Mode("Poly.Close",
       tooltip="Close the border (only useful for non filled polygons)",
       button=Button("Close", "polyclose.png"),
       enter_actions=[Paste("$LCOMMA$.Close[]$RCOMMA$"), update_now])

poly = \
  Mode("Poly",
       tooltip="Create a polygon",
       statusbar=("Select the vertices by clicking on the image view; "
                  "choose the color and filling style from the toolbox"),
       button=Button("Poly", "poly.png"),
       enter_actions=[Paste("$LNEWLINE$\ .Poly[$CURSORIN$]$CURSOROUT$$RNEWLINE$"),
                      push_settings, paste_on_new, update_on_paste],
       exit_actions=pop_settings,
       submodes=[color, gradient, style, poly_smoothing, poly_close, exit])

circle_radius = \
  Mode("Circle radius",
       tooltip="Provide the radius (use twice for ellipse)",
       button=Button("Radius", "circleradius.png"),
       enter_actions=[InputAct("$LCOMMA$$INPUT$$RCOMMA$",
                               label="Radius:")])

circle = \
  Mode("Circle",
       tooltip="Create a new circle",
       button=Button("Circle", "circle.png"),
       enter_actions=[Paste("$LNEWLINE$\ .Circle[$CURSORIN$]$CURSOROUT$$RNEWLINE$"),
                      push_settings, paste_on_new, update_on_paste],
       exit_actions=pop_settings,
       submodes=[color, gradient, style, circle_radius, exit])

line_width = \
  Mode("Line width",
       tooltip="Set the width of the line",
       button=Button("Width", "linewidth.png"),
       enter_actions=[InputAct("$LCOMMA$$INPUT$$RCOMMA$",
                               label="Width of the line:")])

line_sharp = \
  Mode("Sharp",
       tooltip="Line corners are sharp (affects only the following points)",
       button=Button("Sharp", "linesharp.png"),
       enter_actions=Paste("$LCOMMA$line.sharp$RCOMMA$"))

line_smooth = \
  Mode("Smooth",
       tooltip="Line corners are smooth (affects only the following points)",
       button=Button("Smooth", "linesmooth.png"),
       enter_actions=Paste("$LCOMMA$line.medium$RCOMMA$"))

line_vsmooth = \
  Mode("Very-Smooth",
       tooltip="Line corners are very smooth (affects only the following points)",
       button=Button("V.Smooth", "linevsmooth.png"),
       enter_actions=Paste("$LCOMMA$line.smooth$RCOMMA$"))

line_close = \
  Mode("Line.Close",
       tooltip="Close the line",
       button=Button("Close", "lineclose.png"),
       enter_actions=[Paste("$LCOMMA$.Close[]$RCOMMA$"),
                      update_now])

line_cap_arrow =  \
  Mode("Line.Cap.arrow",
       tooltip="Regular arrow",
       button=Button("Arrow", "caparrow.png"),
       enter_actions=[IncludeSrc("caps"),
                      Paste("$LCOMMA$cap_arrow$RCOMMA$"), update_now])

line_cap_triang =  \
  Mode("Line.Cap.triang",
       tooltip="Triangular arrow",
       button=Button("Triangle", "captriang.png"),
       enter_actions=[IncludeSrc("caps"),
                      Paste("$LCOMMA$cap_triangle$RCOMMA$"), update_now])

line_cap_ruler =  \
  Mode("Line.Cap.ruler",
       tooltip="``Ruler'' arrow",
       button=Button("Ruler", "capruler.png"),
       enter_actions=[IncludeSrc("caps"),
                      Paste("$LCOMMA$cap_ruler$RCOMMA$"), update_now])

line_cap_line =  \
  Mode("Line.Cap.line",
       tooltip="Segment cap",
       button=Button("Segment", "caplarr.png"),
       enter_actions=[IncludeSrc("caps"),
                      Paste("$LCOMMA$cap_line_arrow$RCOMMA$"), update_now])

line_cap_circle =  \
  Mode("Line.Cap.circle",
       tooltip="Circle cap",
       button=Button("Circle", "capcircle.png"),
       enter_actions=[IncludeSrc("caps"),
                      Paste("$LCOMMA$cap_circle$RCOMMA$"), update_now])

line_cap =  \
  Mode("Line.Cap",
       tooltip="Select a line cap to insert at the beginning or end of the line",
       button=Button("Cap", "linecaps.png"),
       submodes=[line_cap_arrow, line_cap_triang, line_cap_ruler,
                 line_cap_line, line_cap_circle, exit])

line = \
  Mode("Line",
       tooltip="Create a new line with fixed or variable thickness",
       button=Button("Line", "line.png"),
       enter_actions=[Paste("$LNEWLINE$\ .Line[$CURSORIN$]$CURSOROUT$$RNEWLINE$"),
                      push_settings, paste_on_new, update_on_paste],
       exit_actions=pop_settings,
       submodes=[color, gradient, style, line_width, line_sharp, line_smooth,
                 line_vsmooth, line_cap, line_close, exit])

no_variants = []
default_fonts = \
  (("AvantGarde", "-", ("Book", "BookOblique", "Demi", "DemiOblique")),
   ("Bookman", "-", ("Demi", "DemiItalic", "Light", "LightItalic")),
   ("Courier", "-", (None, "Oblique", "Bold", "BoldOblique")),
   ("Helvetica", "-", (None, "Oblique", "Bold", "BoldOblique")),
   ("Helvetica-Narrow", "-", (None, "Oblique", "Bold", "BoldOblique")),
   ("NewCenturySchlbk", "-", ("Roman", "Italic", "Bold", "BoldItalic")),
   ("Palatino", "-", ("Roman", "Italic", "Bold", "BoldItalic")),
   ("Symbol", None, no_variants),
   ("Times", "-", ("Roman", "Italic", "Bold", "BoldItalic")),
   ("ZapfChancery-MediumItalic", None, no_variants),
   ("ZapfDingbats", None, no_variants))

# Create the modes for all the default fonts (in the table above)
font_modes = []
for font_name, sep, variants in default_fonts:
  variant_modes = []
  for i, variant in enumerate(variants):
    full_name = (font_name + sep + variant
                 if sep != None and variant != None
                 else font_name)
    variant_name = shorten_string(variant) if variant != None else "Normal"
    variant_mode = \
      Mode(full_name,
           tooltip=full_name,
           button=Button(variant_name, "font%s.png" % full_name),
           enter_actions=[Paste("$LCOMMA$\"%s\"$RCOMMA$" % full_name),
                          exit_action])
    variant_modes.append(variant_mode)

  if len(variants) == 0:
    font_mode = \
      Mode(font_name,
           tooltip=font_name,
           button=Button(shorten_string(font_name), "font%s.png" % font_name),
           enter_actions=[Paste("$LCOMMA$\"%s\"$RCOMMA$" % font_name)])

  else:
    variant_modes.append(exit)
    font_mode = \
      Mode(font_name,
           tooltip=font_name,
           button=Button(shorten_string(font_name), "font%s.png" % font_name),
           submodes=variant_modes)

  font_modes.append(font_mode)

font_size = \
  Mode("Font size",
       tooltip="Set the size of the font",
       button=Button("Size", "fontsize.png"),
       enter_actions=[InputAct("$LCOMMA$$INPUT$$RCOMMA$",
                               label="Size of font:")])

font = \
  Mode("Font",
       tooltip="Choose the font (and size) used to render the text",
       button=Button("Font", "font.png"),
       enter_actions=[Paste("$LCOMMA$Font[$CURSORIN$]$CURSOROUT$$RCOMMA$")],
       submodes=[font_size, exit] + font_modes)

text_content = \
  Mode("Text content",
       tooltip="Enter the text to display",
       button=Button("Text", "textcontent.png"),
       enter_actions=[InputAct("$LCOMMA$$INPUT$$RCOMMA$",
                               string_input=True,
                               label="Text to display:")])

text_from_submodes = []
for alignx, aligny in ((1, 1), (0, 0), (2, 0), (0, 2), (2, 2)):
  s = "(%g, %g)" % (alignx*0.5, aligny*0.5)
  idxs = str(alignx) + str(aligny)
  text_from_submode = \
    Mode("From" + idxs,
         tooltip="Position %s" % s,
         button=Button("From" + idxs, "textfrom%s.png" % idxs),
         enter_actions=[Paste(s), exit_action])
  text_from_submodes.append(text_from_submode)
text_from_submodes.append(exit)

text_from = \
  Mode("From",
       tooltip="Choose the relative position",
       button=Button("From", "textfrom.png"),
       enter_actions=Paste("$LCOMMA$.From[$CURSORIN$]$CURSOROUT$$RCOMMA$"),
       submodes=text_from_submodes)

text = \
  Mode("Text",
       tooltip="Insert text into the figure",
       statusbar=("Select the font, enter the text, the position "
                  "and relative-positioning"),
       button=Button("Text", "text.png"),
       enter_actions=[Paste("$LNEWLINE$\ .Text[$CURSORIN$]$CURSOROUT$$RNEWLINE$"),
                      paste_on_new],
       submodes=[color, gradient, style, font, text_content, text_from, exit])

main_mode = \
  Mode("Main", submodes=[exit, color, gradient, style, poly, circle, line, text])

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
