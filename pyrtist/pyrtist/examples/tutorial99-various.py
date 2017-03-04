#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(0.0, 47.4489709172); bbox2 = Point(91.3037539683, -1.781098434)
p1 = Point(10.2040816327, 38.6904453846)
p2 = Point(26.1904761905, 39.2006490635)
p3 = Point(38.2653061224, 38.6904453846)
p4 = Point(55.4421768707, 36.6496306689)
p5 = Point(65.4761904762, 33.7584764883)
p6 = Point(70.0680272109, 44.6428216388)
p7 = Point(83.843537415, 43.9625500669)
p8 = Point(82.6530612245, 36.4795627759)
p9 = Point(76.3605442177, 29.8469149498)
p10 = Point(2.21088435374, 26.7856928763)
p11 = Point(15.8163265306, 28.486371806)
p12 = Point(22.1088435374, 18.7925019064)
p13 = Point(6.46258503401, 10.2891072575)
q1 = Point(32.8231292517, 29.6768470569)
q2 = Point(26.0204081633, 14.370736689)
q3 = Point(43.1972789116, 16.2414835117)
q4 = Point(54.9319727891, 14.8809403679)
q5 = Point(45.9183673469, 23.7244708027)
q6 = Point(57.9931972789, 24.0646065886)
q7 = Point(48.8095238095, 16.2414835117)
q8 = Point(61.9047619048, 12.3299219732)
q9 = Point(66.156462585, 25.5952176254)
q10 = Point(72.2789115646, 16.2414835117)
q11 = Point(78.0612244898, 24.0646065886)
q12 = Point(77.8911564626, 12.4999898662)
q13 = Point(88.3503401361, 18.7074925426)
q14 = Point(12.8867030612, 4.80486655518)
q15 = Point(33.961772449, 5.29881404682)
q16 = Point(51.5792132653, 4.6402173913)
q17 = Point(64.0925357143, 0.688637458194)
#!PYRTIST:REFPOINTS:END
# TUTORIAL EXAMPLE N.3
# Usage examples for Circle, Poly, Line and Text

from pyrtist.lib2d import *

fg = Window()
fg.take(BBox(bbox1, bbox2))

# Some examples of usage of Circle, Circles, Poly, Line, and
# Text are provided here. These five objects allow tracing quite
# a number of shapes. To get further help on a specific object
# try the help browser: menu Help -> Documentation Browser 
# or just type CTRL+H.

fg.take(
  Circle(p1, 8),                 # Circle
  Circle(p2, 4, 8),              # Ellipse
  Circles(p3, 3, 4, 5, 6),       # Rings
  Circles(p4, Radii(3, 6),
          p4, Radii(6, 3)),      # Crossing ellipses
  Poly(p5, p6, p7, p8, p9),      # Polygon
  Poly(p10, p11, 0.5, p12, p13), # Partially rounded polygon
  Poly(0.5, p10, p11, p12, p13,
       Color.red),               # Partially rounded polygon (red)
  Poly(q1, q2, q3),                  # Polygon
  Poly(0.3, q1, q2, q3, Grey(0.5)),  # Same, rounding 0.3
  Poly(0.5, q1, q2, q3, Grey(0.8)),  # Same, rounding 0.5
)

# Dashed stroke style
ss1 = StrokeStyle(1, Dash(3, 1))

# Stroke style for lines: using round caps and round joints
ss2 = StrokeStyle(Cap.round, Join.round)

fg.take(
  Line(0.5, q4, q5, q6, q7),          # Simple line with width 0.5
  Line(ss1, q8, q9, q10),             # Dashed line
  Line(5, q11, q12, q13, ss2),        # Thick line with round cap
  Text(Font(3), "Hello\nworld!", q14),
  Text(Font("times", 8), "BOX^{ER}", q15),
  Text(Font("times", 8), "_xx^x", q16),
  Text(Font("times", 6), "ONE\nTWO", q17, Offset(0, 0))
)

gui(fg)
