#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(0.0, 50.0); bbox2 = Point(100.0, 12.5838926174)
p1 = Point(3.15540458874, 46.942241204)
p2 = Point(3.23537580547, 42.1395946309)
p4 = Point(28.5119375629, 38.1285583893)
q1 = Point(73.1545885714, 21.8120805369)
q3 = Point(93.6244457143, 38.4228187919)
q5 = Point(66.4133738602, 33.8755592617)
q6 = Point(94.2249240122, 24.9089847651)
q7 = Point(84.8024316109, 26.7326948322)
q2 = Tri(q5, Point(70.1344457143, 37.2483221477))
q4 = Tri(q6, Point(90.4365171429, 20.1342281879), q7)
#!PYRTIST:REFPOINTS:END
# TUTORIAL EXAMPLE N.3
# How to create a closed figure using bezier curves.

from pyrtist.lib2d import *

w = Window()

s = "             "
font = Font(2, "Helvetica")
w.take(
  Text(p1, font, Offset(0, 1), Color.red,
       "Pyrtist allows creating curves (cubic Bezier splines)."
       "\nThis example explains how."),
  Text(p2, font, Offset(0, 1),
       "STEP 1: launch Pyrtist or create a new document (CTRL+N)\n",
       "STEP 2: click on the button       to create a new curved polygon\n",
       "STEP 3: move the mouse where you want to create the first vertex.\n",
       s, "Click on the left button of the mouse\n",
       "STEP 4: repeat step 3 to create other 3 vertices. You should see\n",
       s, "a black polygon with straight boundaries\n",
       "STEP 5: move the mouse over one of the vertices. Press the CTRL\n",
       s, "key and the left mouse button, simultaneously. Keep them\n",
       s, "pressed while moving the mouse out of the vertex. A round\n",
       s, "reference point appears and the polygon edge is rounded.\n",
       "STEP 6: you can repeat step 5 for the same vertex or for other\n",
       s, "vertices. You should obtain something similar to what shown on\n",
       s, "the left")
)

# The line below is what draws the curved polygon.       
w.take(Curve(q1, q2, q3, q4))
w.take(Image("curve.png", p4, 2.5))
w.BBox(bbox1, bbox2)

gui(w)
