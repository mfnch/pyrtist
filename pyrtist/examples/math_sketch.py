#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
axis1 = Point(45.68537543081555, 22.500506912573893)
bbox1 = Point(-3.210913657391826, 24.051235044230708)
bbox2 = Point(49.817079679961466, -6.996937701211181)
origin1 = Point(16.50811104009687, -0.1497825072322314)
p6 = Point(1.8732865480355372, -5.121394918090672)
p8 = Point(4.99437083725779, 15.94642663797599)
p9 = Point(34.09659033571045, 13.908939751665095)
question1 = Point(29.588359236303056, 17.490474510018924)
p3 = Tri(p6, Point(0.8250323768837546, 1.2245005760562506))
p4 = Tri(p8, Point(17.212571060952634, 18.819267346861547))
p5 = Tri(p9, Point(35.05620227563241, 4.5656202170512685))
#!PYRTIST:REFPOINTS:END
from pyrtist.lib2d import *

w = Window()             # Create a new Window.
w << BBox(bbox1, bbox2)  # Give bounding box (containing bbox1, bbox2) to w.

# Thicknesseses
class T:
  axis = 0.18
  vector = axis * 2
  point = axis * 3.0
  curve = vector

w << Color.black
font = Font('times', 1.5)

# Axes
t = 0.18
x_axis, y_axis = (Point(axis1.x, origin1.y), Point(origin1.x, axis1.y))
w << Line(T.axis, origin1, x_axis, Scale(1.5), arrows.line)
w << Line(T.axis, origin1, y_axis, Scale(1.5), arrows.line)
w << Text(font, origin1, "O", Offset(1.5, 1.5))
w << Text(font, x_axis, "x", Offset(0, 1.5))
w << Text(font, y_axis, "y", Offset(-0.2, 0))

curve = Curve(p3, p4, p5, Close.no)
w << Grey(0.65)
w << Stroke(StrokeStyle(T.curve, Dash(0.1, 0.7), Cap.round), Path(curve))
w << Text(Font('palatino', 5), '?', question1, Offset(0, 0))

# Velocity vectors
w << Color(0.4, 0.8, 0.3)
w << Line(T.vector, p3.p, p3.op, arrows.triangle)
w << Line(T.vector, p5.p, p5.op, arrows.triangle)
w << Circle(p3.p, T.point)
w << Circle(p5.p, T.point)
w << Text(font, 0.5 * (p3.p + p3.op), "v_I", Offset(1.3, 1.5))
w << Text(font, 0.5 * (p5.p + p5.op), "v_F", Offset(1.3, 1.5))

# Position vectors
w << Color(1.0, 0.4, 0.3)
w << Line(T.vector, origin1, p3.p, arrows.triangle)
w << Text(font, 0.5 * (p3.p + origin1), "r_I", Offset(0.5, 1.5))

w << Line(T.vector, origin1, p5.p, arrows.triangle)
w << Circle(origin1, T.point)
w << Text(font, 0.5 * (p5.p + origin1), "r_F", Offset(0.5, 1.5))

w.save('math_sketch.svg', width=20.0, unit='cm')

gui(w)
