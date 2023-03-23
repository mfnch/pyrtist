#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
p1 = Point(0.1, 0.5); p2 = Point(0.4, 0.9); p3 = Point(0.6, 0.1)
p4 = Point(0.9, 0.5)
#!PYRTIST:REFPOINTS:END
# This example shows how to draw using the cairo API
# The example was adapted from: https://pycairo.readthedocs.io/en/latest/
import cairo

WIDTH, HEIGHT = (400, 400)
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, WIDTH, HEIGHT)
context = cairo.Context(surface)
context.scale(WIDTH, -HEIGHT)  # Flip the y axis
context.translate(0, -1)       # To align with the GUI's coordinates
context.set_line_width(0.04)
context.move_to(p1.x, p1.y)
context.curve_to(p2.x, p2.y, p3.x, p3.y, p4.x, p4.y)
context.stroke()
context.set_source_rgba(1, 0.2, 0.2, 0.6)
context.set_line_width(0.02)
context.move_to(p1.x, p1.y)
context.line_to(p2.x, p2.y)
context.move_to(p3.x, p3.y)
context.line_to(p4.x, p4.y)
context.stroke()

#surface.write_to_png("example.png")  # Output to PNG

#----------------------------------------------------------------------------
# The code below is just to let the Pyrtist GUI visualize the cairo surface.
from pyrtist.lib2d import *
w = Window()
pat = Image(surface, Extend.none, Filter.best, Offset(0.0, 0.0))
bb1 = Point(0.0, 0.0)
bb2 = Point(1.0, 1.0)
w << Rectangle(bb1, bb2, Style(pat))
w << BBox(bb1, bb2)
gui(w)
