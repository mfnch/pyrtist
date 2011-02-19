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

import os
import sys

import pygtk
pygtk.require('2.0')
import gtk
import gtk.glade
import gtk.gdk

sys.path.append("..")
from config import icon_path
from toolbox import ToolBox
import assistant
import boxmode


class MyWin(object):
  def __init__(self, gladefile="boxer.glade", filename=None, box_exec=None):
    builder = gtk.Builder()
    builder.add_from_file("./e2.glade")
    sigs = {"on_experiment_destroy": gtk.main_quit}

    builder.connect_signals(sigs)
    self.window = builder.get_object("experiment")
    self.hpaned = builder.get_object("hpaned1")
    self.statusbar = builder.get_object("statusbar")
    self.textview = self.hpaned.get_children()[0]

    # Create the assistant
    self.assistant = assistant.Assistant(boxmode.main_mode)
    self.assistant.start()
    self.assistant.set_textview(self.textview)
    self.assistant.set_textbuffer(self.textview.get_buffer())
    self.assistant.set_statusbar(self.statusbar)

    self.toolview = ToolBox(self.assistant, icon_path=icon_path)
    self.hpaned.remove(self.textview)
    self.hpaned.pack1(self.toolview, shrink=False)
    self.hpaned.pack2(self.textview, shrink=False)
    self.window.show_all()

  def zoom_inc_clicked(self, *v):
      self.box_window.zoom_in()



class Config:
  use_threads = False

config = Config()

def run(filename=None, box_exec=None):
  if config.use_threads:
    gtk.gdk.threads_init()
    gtk.gdk.threads_enter()

  main_window = MyWin()
  gtk.main()

  if config.use_threads:
    gtk.gdk.threads_leave()

run()
