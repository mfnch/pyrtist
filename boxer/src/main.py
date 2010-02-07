# Copyright (C) 2010
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

import optparse

import info, config, boxer

option_parser = None

def generate_option_parser():
  usage = ("boxer [options] [filename.box]") 
  version = ("%s -- The Box Graphical User Interface\n" % info.full_name +
             "Copyright (C) 2010 Matteo Franchin\n")

  op = optparse.OptionParser(usage=usage, version=version)

  help = ("The executable of the Box compiler to be used. "
          "If the file is found and can be executed correctly, Boxer "
          "remembers it in future sessions.")
  op.add_option("--box-exec", type="string", metavar="EXEC-FILE",
                dest="box_exec", help=help)

  return op

def main(args):
  global option_parser
  if option_parser == None:
    option_parser = generate_option_parser()

  (options, arguments) = option_parser.parse_args(args)

  file_to_edit = None
  if len(arguments) >= 2:
    file_to_edit = arguments[1]

  if len(arguments) > 2:
    other_files = ", ".join(arguments[2:])
    print ("WARNING: Boxer takes just one file from the command line. "
           "Ignoring the files %s." % other_files)

  boxer.run(file_to_edit, box_exec=options.box_exec)

if __name__ == "__main__":
  import sys
  main(sys.argv)

