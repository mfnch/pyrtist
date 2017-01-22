#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN

#!PYRTIST:REFPOINTS:END
# One example taken from the samples page of the Cairo graphic library
# (http://cairographics.org/samples/), rewritten for Pyrtist.

from pyrtist.lib2d import *

w = Window()

pat1 = Gradient(Line((0, 256), (0, 0)), Color.white, Color.black)
pat2 = Gradient(Circles(Point(115.2, 153.6), 25.6,
                        Point(102.4, 153.6), 128.0),
                Color.white, Color.black)
w.take(
  Poly((0, 0), (256, 0), (256, 256), (0, 256), pat1),
  Circle((128, 128), 76.8, pat2)
)
#w.save("cairo_gradient.png", mode="rgb24", resolution=1)

gui(w)
