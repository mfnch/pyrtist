#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(0, 50); bbox2 = Point(100, 0)
p1 = Point(25.7575757576, 30.3787878788)
p2 = Point(19.814314022, 27.0345941837)
p3 = Point(27.7318108704, 19.2297513487)
p4 = Point(47.8787878788, 18.4090909091)
p5 = Point(67.7272727273, 24.7727272727)
p6 = Point(49.3939393939, 36.2878787879)
p7 = Point(28.4848484848, 38.8636363636); p8 = Point(40.4545454545, 42.5)
p9 = Point(38.9393939394, 35.9848484848)
#!PYRTIST:REFPOINTS:END
# Example showing how to draw arcs and compute their bounding box.
from pyrtist.lib2d import *

w = Window()
w << BBox(bbox1, bbox2)

# Define three arcs.
arc1 = Arc(Through(p1, p2, p3), Close.no)
arc2 = Arc(Through(p4, p5, p6), Color.red, StrokeStyle(1, Color.blue, Cap.round))
arc3 = Arc(Through(p7, p8, p9))

# Draw them.
w << Stroke(StrokeStyle(0.5), Path(arc1)) << arc2 << arc3

# Find the bounding box of arc3 and draw it as a rectangle.
bb = BBox(arc3)
w << Stroke(0.2, Color.red, Rectangle(*bb))

# Find the point in the middle of arc1 using 0.5 as an index.
w << Circle(Color(0, 0.5, 0), 1, arc1[0.5])

gui(w)
