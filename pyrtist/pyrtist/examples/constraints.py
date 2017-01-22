#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(0, 50); bbox2 = Point(100, 0); p1 = Point(45.0, 13.2575757576)
p2 = Point(29.8484848485, 39.1666666667)
p3 = Point(70.3030303031, 27.4999999999)
p4 = Point(34.5827690377, 31.0710407031)
p5 = Point(78.8827259507, 22.3629947834)
#!PYRTIST:REFPOINTS:END
from pyrtist.lib2d import *

w = Window()
w.BBox(bbox1, bbox2)

w.take(Line(p1, p2, 0.3))
radius = 10.0
w.take(Circle(p3, radius, Style(Color.none, Border(0.3, Color.black))))

# Project p4 onto the line.
v = (p2 - p1).normalized()
p4 = p1 + v*((p4 - p1).dot(v))

# Project p5 onto the circle.
p5 = p3 + radius*(p5 - p3).normalized()

gui.move(p4=p4, p5=p5)
gui(w)
