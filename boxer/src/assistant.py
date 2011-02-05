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

class Action(object):
  pass

class Paste(Action):
  def __init__(self, text):
    self.text = text

class Set(Action):
  def __init__(self, property_name, value):
    self.property_name = property_name
    self.value = value

class Button(object):
  def __init__(self, filename):
    self.filename = filename

class Mode(object):
  def __init__(self, name, button=None, enter_actions=None, exit_actions=None,
               permanent=False, submodes=None):
    self.name = name
    self.permanent = True
    self.button = button
    self.enter_actions = enter_actions
    self.exit_actions = exit_actions
    self.submodes = submodes

class Assistant(object):
  def __init__(self, main_mode):
    self.main_mode = main_mode
    self.current_modes = [main_mode]
    self.permanent_modes = filter(lambda m: m.permanent, main_mode.submodes)

  def start(self):
    """Set (or reset) the mode to the main one."""
    self.current_modes = [self.main_mode]

  def choose(self, new_mode):
    """Choose a submode from the ones available in the current mode."""
    modes = self.get_available_modes()
    mode_names = map(lambda m: m.name, modes)
    try:
        mode_idx = mode_names.index(new_mode)
        self.current_modes.append(modes[mode_idx])

    except ValueError:
      raise ValueError("Mode not found (%s)" % new_mode)

  def exit_mode(self):
    if len(self.current_modes) > 1:
      self.current_modes.pop(-1)

  def get_available_modes(self):
    """Return the modes currently available."""
    if len(self.current_modes) > 0:
      current_mode = self.current_modes[-1]
      return (current_mode.submodes if current_mode.submodes != None else [])

    else:
      return []

  def get_available_mode_names(self):
    """Return the names of the modes currently available."""
    return map(lambda m: m.name, self.get_available_modes())

#============================================================================#
color = \
  Mode("Color",
       permanent=True,
       button=Button("color.png"),
       enter_actions=Paste("Color[$CURSORIN$]$CURSOROUT$"))

style = Mode("Style",
             permanent=True)

poly = \
  Mode("Poly",
       button=Button("poly.png"),
       enter_actions=[Paste("Poly[$CURSORIN$]\n$CURSOROUT$"),
                      Set("PastePoint", True)],
       submodes=[color, style])

circle = \
  Mode("Circle",
       button=Button("circle.png"),
       enter_actions=[Paste("Circle[$CURSORIN$]\n$CURSOROUT$"),
                      Set("PastePoint", True)],
       submodes=[color, style])

line = \
  Mode("Line",
       button=Button("line.png"),
       enter_actions=[Paste("Line[$CURSORIN$]\n$CURSOROUT$"),
                      Set("PastePoint", True)],
       submodes=[color, style])

text = \
  Mode("Text",
       button=Button("text.png"),
       enter_actions=[Paste("Text[$CURSORIN$]\n$CURSOROUT$"),
                      Set("PastePoint", True)],
       submodes=[color, style])

main_mode = \
  Mode("Main", submodes=[color, style, poly, circle, line, text])

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
