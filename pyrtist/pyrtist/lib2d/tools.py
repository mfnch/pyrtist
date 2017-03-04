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

__all__ = ('Bar', 'Sphere')

from .base import *
from .core_types import *
from .style import *
from .window import Window
from .line import Line
from .poly import Poly
from .gradient import Gradient
from .circle import Circle, Circles
from .text import Text


class Drawable(Taker): pass

@combination(Drawable, Window)
def drawable_at_window(drawable, window):
    drawable.draw(window)


class Bar(Drawable):
    '''A progress bar.'''

    default_bg_style = Style(Grey(0.8), Border(Color.black, 0.5))
    default_fg_color = Color.blue
    default_pos = Point(0.0, 0.0)
    default_size = Point(50.0, 5.0)
    clone_translation = Point(0.0, 6.0)

    def __init__(self, interval=(0.0, 1.0), value=None,
                 pos=None, size=None, cursor=None,
                 fg_color=None, bg_style=None, label='{:.2f}', font=None):
        super(Bar, self).__init__()
        self.interval = interval
        self.pos = pos or self.default_pos
        self.size = size or self.default_size
        self.font = font or Font(self.size.y)
        self.cursor = cursor or (self.pos + Point(0.5*self.size.x))
        self.fg_color = fg_color or self.default_fg_color
        self.bg_style = bg_style or self.default_bg_style
        self.label = label
        Bar.default_pos = self.pos + Bar.clone_translation

    @property
    def value(self):
        v = min(1.0, max(0.0, (self.cursor - self.pos).x / self.size.x))
        i0, i1 = self.interval
        return i0 + v*(i1 - i0)

    def draw(self, window):
        p = self.pos
        q = self.pos + self.size
        px = min(q.x, max(p.x, self.cursor.x))
        ps = [p, Point(q.x, p.y), q, Point(p.x, q.y)]
        g = Gradient(Line(ps[0], ps[1]),
                     self.fg_color, Color.white, self.fg_color)
        window.take(
            Poly(self.bg_style, *ps),
            Poly(ps[0], Point(px, p.y), Point(px, q.y), ps[3], g),
            Poly(self.bg_style, Color.none, *ps),
            Text(Color.black, self.font, 0.5*(p + q),
                 self.label.format(self.value))
        )


class Sphere(Drawable):
    '''Circle painted using a gradient to mimic 3D lighting on a sphere.'''

    def __init__(self, *args):
        self.center = Point()
        self.radius = 1.0
        self.color = Color.black
        super(Sphere, self).__init__(*args)

    def draw(self, window):
        cs = Circles(self.center + self.radius*Point(-0.2, 0.2),
                     0.0, self.radius)
        g = Gradient(cs, Color.white, 0.4, Color.white, self.color)
        window.take(Circle(self.center, self.radius, g))

@combination(Sphere, Sphere)
def sphere_at_sphere(lhs, rhs):
    rhs.center = lhs.center
    rhs.radius = lhs.radius
    rhs.color = lhs.color

@combination(Point, Sphere)
def point_at_sphere(point, sphere):
    sphere.center = point

@combination(int, Sphere)
@combination(float, Sphere)
def point_at_sphere(real, sphere):
    sphere.radius = real

@combination(Color, Sphere)
def point_at_sphere(color, sphere):
    sphere.color = color
