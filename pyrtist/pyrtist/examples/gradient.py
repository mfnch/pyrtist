#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN

#!PYRTIST:REFPOINTS:END
from pyrtist.lib2d import *

w = Window()

# Define a linear and radial gradient.
g1 = Gradient(Line((-25, -25), (0, 0)), Color.red, Color.yellow)
g2 = Gradient(Circles(Point(10, 5), 0, 12), Color.white, Color.black)

ball = Window(Circle(g2, Point(10, 5), 12))

w.take(
  Poly(g1, (-25, -25), (25, -25), (25, 25), (-25, 25)),
  Put(ball),  # Just place the ball, as it is.
  Put(ball, Point(-15, -10), Scale(0.2, 0.5), AngleDeg(45))  # Scale and rotate it.
)

gui(w)

#w.save("gradient.png")
