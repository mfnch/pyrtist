# Copyright (C) 2017 by Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Pyrtist.
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

__all__ = ('Gradient',)

from .base import Taker, combination
from .core_types import Point
from .pattern import Pattern, Extend
from .style import Color
from .line import Line
from .circle import Circles
from .cmd_stream import CmdStream, Cmd


class ColorStop(object):
    '''Object representing a color shade in a gradient.'''

    def __init__(self, offset=None, color=None):
        '''Create a new ColorStop object.

        offset: Can either be a float or None. In the latter case, the offset
                is automatically deduced.
        color:  The color to use at this offset in the gradient.
        '''
        self.offset = offset
        self.color = color

    def __repr__(self):
        return 'ColorStop({}, {})'.format(self.offset, self.color)

    def copy(self):
        return type(self)(offset=self.offset, color=self.color)


class ColorStops(Taker):
    def __init__(self):
        super(ColorStops, self).__init__()

        # Tuple of ColorStop objects.
        self.color_stops = []

    def add(self, color_stop):
        assert isinstance(color_stop, ColorStop)
        self.color_stops.append(color_stop)

    def get_color_stops(self):
        # Find all contiguous groups of stop colors with None offsets.
        groups = []
        current_group = None
        for idx, color_stop in enumerate(self.color_stops):
            if color_stop.offset is None:
                if current_group is None:
                    current_group = [idx, idx]
                    groups.append(current_group)
                else:
                    current_group[1] = idx
            else:
                current_group = None


        if len(groups) == 0:
            return []

        # Replace all None offsets with automatically computed offsets.
        out = [color_stop.copy() for color_stop in self.color_stops]
        max_idx = len(out) - 1
        for start_idx, end_idx in groups:
            if start_idx == 0:
                # Initial offset was not provided. Assume this is zero.
                out[start_idx].offset = 0.0
                start_idx += 1
            start_offset = out[start_idx - 1].offset

            if end_idx == max_idx:
                # End offset was not provided. Assume this is one.
                out[end_idx].offset = 1.0
                end_idx -= 1
            end_offset = out[end_idx + 1].offset

            # Linearly interpolate the missing offsets.
            num_stops = end_idx - start_idx + 2
            offset = start_offset
            delta = (end_offset - start_offset)/num_stops
            for idx in range(start_idx, end_idx + 1):
                offset += delta
                out[idx].offset = offset

        return out

    def _get_cmds(self):
        return [Cmd(Cmd.pattern_add_color_stop_rgba, color_stop.offset,
                    color_stop.color)
                for color_stop in self.get_color_stops()]


@combination(Color, ColorStops)
def color_at_color_stops(color, color_stops):
    if len(color_stops.color_stops) > 0:
        last_color_stop = color_stops.color_stops[-1]
        if last_color_stop.color is None:
            last_color_stop.color = color
            return
    color_stops.color_stops.append(ColorStop(color=color))

@combination(int, ColorStops)
@combination(float, ColorStops)
def offset_at_color_stops(offset, color_stops):
    if not (0.0 <= offset <= 1.0):
        raise ValueError('Color offset should be between 0 and 1 inclusive')
    if len(color_stops.color_stops) > 0:
        last_color_stop = color_stops.color_stops[-1]
        if last_color_stop.color is None:
            last_color_stop.offset = offset
            return
    color_stops.color_stops.append(ColorStop(offset=offset))


class Gradient(Pattern, Taker):
    def __init__(self, *args):
        Pattern.__init__(self)
        Taker.__init__(self)
        self.shape = None
        self.color_stops = ColorStops()
        self.pattern_extend = None
        self.take(*args)

    def _get_cmds(self):
        cmds = CmdStream()
        if isinstance(self.shape, Line):
            p1, p2 = self.shape
            cmds.take(Cmd(Cmd.pattern_create_linear, p1, p2))
        elif isinstance(self.shape, Circles):
            (ctr1, r1), (ctr2, r2) = self.shape._get_tuples()
            refx = Point(ctr1.x + 1.0, ctr1.y)
            refy = Point(ctr1.x, ctr1.y + 1.0)
            refr1 = Point(ctr1.x + r1, ctr1.y)
            refr2 = Point(ctr1.x + r2, ctr1.y)
            cmds.take(Cmd(Cmd.pattern_create_radial,
                          ctr1, refx, refy, ctr2, refr1, refr2))
        else:
            raise ValueError('Invalid shape object for Gradient')
        cmds.take(*self.color_stops._get_cmds())
        cmds.take(self.pattern_extend)
        cmds.take(Cmd(Cmd.pattern_set_source))
        return cmds.cmds


@combination(Line, Gradient)
def line_at_gradient(line, gradient):
    if len(line) != 2:
        raise ValueError('Gradient takes Line with exactly 2 points '
                         '({} given)'.format(len(line)))
    gradient.shape = line

@combination(Circles, Gradient)
def circles_at_gradient(circles, gradient):
    if len(circles) != 2:
        raise ValueError('Gradient takes Circles with exactly 2 circles '
                         '({} given)'.format(len(circles)))
    gradient.shape = circles

@combination(Color, Gradient)
@combination(int, Gradient)
@combination(float, Gradient)
def color_or_offset_at_gradient(color_or_offset, gradient):
    gradient.color_stops.take(color_or_offset)

@combination(Extend, Gradient)
def extend_at_gradient(extend, gradient):
    gradient.pattern_extend = extend
