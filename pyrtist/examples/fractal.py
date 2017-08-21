#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(-23.0957515662, 79.6991129258)
bbox2 = Point(71.3423454797, -1.88531784342)
p1 = Point(-20.2154615182, 30.6496131137)
p2 = Point(-2.9523785489, 65.0652930926)
p3 = Point(35.303325298, 77.1040972766)
p4 = Point(69.2944192672, 43.4906900368)
p5 = Point(25.8613547511, 9.49960406392)
v1 = Point(52.0608243954, 2.61783485327)
#!PYRTIST:REFPOINTS:END
from pyrtist.lib2d import *
from pyrtist.lib2d.tools import Bar

w = Window()
w << BBox(bbox1, bbox2)

def draw_fractal(dest, points, color, d, n, level=0):
  poly = Poly(color, *points) >> dest
  new_points = [poly[i + d] for i, p in enumerate(poly)]
  new_color = color.darken(1.0 - 5.0/n)
  if level > 0:
    draw_fractal(dest, new_points, new_color, d, n, level - 1)

bar_r = Bar(cursor=v1, interval=(10, 300), label='level={:.0f}') >> w
n = int(bar_r.value)

w << BBox(bbox1, bbox2)
draw_fractal(w, [p1, p2, p3, p4, p5], Color.red, 5.0/n, n, n)

gui(w)
