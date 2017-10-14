# Copyright (C) 2017 Matteo Franchin
#
# This file is part of Pyrtist.
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

from .core_types import Taker, combination
from .cmd_stream import CmdStream

class Path(Taker):
    def __init__(self, *args):
        super(Path, self).__init__()
        self.cmd_stream = CmdStream()
        self.take(*args)

@combination(Path, Path)
def path_at_path(child, parent):
    parent.cmd_stream.take(child.cmd_stream)

@combination(Path, CmdStream)
def path_at_cmd_stream(path, cmd_stream):
    cmd_stream.take(path.cmd_stream)
