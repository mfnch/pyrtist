#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(-33.3736933815, 87.8159051329)
bbox2 = Point(76.1030045728, -1.65034200668)
gui1 = Point(34.5696427817, 2.57241672105)
#!PYRTIST:REFPOINTS:END
from pyrtist.lib2d import *
from pyrtist.lib2d.tools import Bar

# We define a new object, which contains all the details about
# the Pythagorean tree
def draw_tree(dest, level, p1, p2, xmid, h, d=1e-10):
    g = 1.0 / (level + 1)
    b = 1 - g
    c = Color(0.5*b, g, 0)

    side = Line(p1, p2)
    p = Poly(side[0], side[1 - d], side[1 - d, 1], side[0, 1], c) >> dest
    t = Line(p[2], p[3], p[2 + xmid, -h])

    if level >= 1:
        draw_tree(dest, level - 1, t[2], t[0], xmid, h, d)
        draw_tree(dest, level - 1, t[1], t[2], xmid, h, d)

w = Window()
w << BBox(bbox1, bbox2)

# Now we can draw the tree with the following line
gui_r = Bar(cursor=gui1, interval=(0, 15), label='level={:.0f}') >> w

level = int(gui_r.value)
draw_tree(w, level, Point(5, 10), Point(25, 10), 0.6, 0.45)

#w.save('fractree.png')
gui(w)

