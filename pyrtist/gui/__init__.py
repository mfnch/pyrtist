# Copyright (C) 2017, 2020, 2023 Matteo Franchin
#
# This file is part of Pyrtist.
#
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 2.1 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

import sys
import optparse

from .mainwindow import run
from .info import full_name


def generate_option_parser():
  usage = ("pyrtist [options] [filename.py]")
  version = ("{} -- Draw using Python!\n"
             "Copyright (C) 2010-2017 Matteo Franchin"
             .format(full_name))
  op = optparse.OptionParser(usage=usage, version=version)
  return op

def main(args=None):
  option_parser = generate_option_parser()
  (options, arguments) = option_parser.parse_args(args or sys.argv)

  file_to_edit = None
  if len(arguments) >= 2:
    file_to_edit = arguments[1]

  if len(arguments) > 2:
    other_files = ", ".join(arguments[2:])
    sys.stdout.write("WARNING: Pyrtist takes just one file from the command "
                     "line. Ignoring the files %s." % other_files)

  run(file_to_edit)
