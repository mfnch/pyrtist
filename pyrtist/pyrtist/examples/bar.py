#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
gui1 = Point(19.4886046743, 3.84443340587)
gui2 = Point(-3.56565488372, 9.39682833333)
gui3 = Point(53.0555553922, -4.96428689655)
#!PYRTIST:REFPOINTS:END
from pyrtist.lib2d import *

class Bar(object):
    def __init__(self, size=Point(50.0, 8.0), completed=0.0,
                 bg_color=None, fg_color=None):
        self.size = size
        self.completed = completed
        self.bg_color = bg_color or Color.grey
        self.fg_color = fg_color or Color.blue

    def draw(self, w):
        lx, ly = self.size
        px = lx * min(1.0, max(0.0, self.completed))
        ps = [Point(0, 0), Point(lx, 0), Point(lx, ly), Point(0, ly)]
        g = Gradient(Line(ps[0], ps[1]), self.fg_color, Color.white, self.fg_color)
        w.take(Poly(self.bg_color, *ps),
               Poly(ps[0], Point(px, 0), Point(px, ly), ps[3], g),
               Poly(Style(Color.none, Border(Color.black, 0.2)), *ps))

# Example using the Bar object
w = Window()
w.BBox(gui2, gui3)

b = Bar(Point(50, 5), completed=gui1.x/50.0)
b.draw(w)

gui(w)
