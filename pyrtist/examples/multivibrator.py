#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(-1.78114006536, 62.385); bbox2 = Point(76.8434514839, -2.13)
#!PYRTIST:REFPOINTS:END
# Here we show how to draw a small electric circuit.

from pyrtist.lib2d import *
from pyrtist.lib2d.prefabs.electric import *

# These parameters control the spacing of the circuit.
sx, sy = (15.0, 30.0)  # scales along x and y directions.
# x offsets and y offsets.
x0, x1, x2, x3, x4 = [sx*x for x in (0.0, 1.0, 2.0, 3.5, 4.5)]
y0, y1, y2 = [sy*y for y in (0.0, 1.0, 2.0)]

w = Window()

# Place the four resistors, the two condenser and the two NPN transistors
# The string "rt" means: automatically rotate and translate such that
# the Near() constraints are better satisfied.
r1 = Put(resistor, "rt", Near(0, (x1, y0)), Near(1, (x1, y1))) >> w
r2 = Put(resistor, "rt", Near(0, (x2, y0)), Near(1, (x2, y1))) >> w
r3 = Put(resistor, "rt", Near(0, (x3, y0)), Near(1, (x3, y1))) >> w
r4 = Put(resistor, "rt", Near(0, (x4, y0)), Near(1, (x4, y1))) >> w
c1 = Put(capacitor, "rt", Near(0, (x1, y1)), Near(1, (x2, y1))) >> w
c2 = Put(capacitor, "rt", Near(0, (x4, y1)), Near(1, (x3, y1))) >> w
t1 = Put(transistor_NPN, "rt", Near("C", (x1, y1)), Near("E", (x1, y2))) >> w
t2 = Put(transistor_NPN, "rt", Scale(1, -1),
         Near("C", (x4, y1)), Near("E", (x4, y2))) >> w

# Trace all the connections between the components.
segments = \
  [((x0, y0), (x4, y0), r4[0]),
   ((x1, y0), r1[0]),
   ((x2, y0), r2[0]),
   ((x3, y0), r3[0]),
   (r1[1], t1["C"]), (r4[1], t2["C"]),
   (t1["E"], (x1, y2)), (t2["E"], (x4, y2), (x0, y2)),
   (r2[1], (x2, y1), c1[1]), (c1[0], (x1, y1)),
   (r3[1], (x3, y1), c2[1]), (c2[0], (x4, y1)),
   ((x2, y1), t2["B"] - Point(sx*0.2, 0), t2["B"]),
   ((x3, y1), t1["B"] + Point(sx*0.2, 0), t1["B"])]

w << Args(Line(0.4, Args(Point(*p) for p in ps))
          for ps in segments)

# Insert small circles, where more than 3 lines meet.
w << Circles(0.6, Point(x1, y0), Point(x2, y0), Point(x3, y0), Point(x1, y2),
             Point(x2, y1), Point(x3, y1), Point(x1, y1), Point(x4, y1))

# Some labels near the components
#w << Texts(Font("Helvetica", 4.0),
#        "T1", t1[0.5], Offset(2.1, 0.5);
#        "R1", r1[0.5];
#        "R2", r2[0.5], Offset(1.9, 0.5);
#        "T2", t2[0.5], Offset(-0.9, 0.5);
#        "R3", r3[0.5];
#        "R4", r4[0.5];
#        "C1", c1[0.5], Offset(0.5, -1.5);
#        "C2", c2[0.5])

w << BBox(bbox1, bbox2)

#w.save('multivibrator.png')
gui(w)
