#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(15.6225412606, 30.7384856532)
bbox2 = Point(38.0423256326, 11.8849630531)
bbox3 = Point(-13.8721863036, 71.6629154857)
bbox4 = Point(86.7246167767, -10.7160646028)
p1 = Point(17.4669426557, 38.4105960265)
p2 = Point(16.6067117105, 28.0125458067)
p3 = Point(19.6192672026, 21.8148990154)
p4 = Point(35.5133561647, 22.5165562914)
p5 = Point(49.4206840066, 22.5165562914)
p6 = Point(56.2087845008, 24.5033112583)
p7 = Point(55.8776576474, 32.119205298)
p8 = Point(54.5531502339, 40.2317880795)
p9 = Point(41.8047663789, 39.4039735099)
p10 = Point(25.5795505634, 39.4039735099); p11 = Point(36.159656, 29.967347351)
p12 = Point(16.977676112, 10.6486261589)
p13 = Point(57.4495143196, 11.0927152318)
p14 = Point(12.4519598622, 22.5657303129)
p15 = Point(11.5978402783, 40.1604624732)
p16 = Point(26.0608908833, 40.0465806148)
p17 = Point(26.3455941817, 22.7080815671)
p18 = Point(27.7910894389, 22.2810307033)
p19 = Point(27.4874049505, 20.2311572519)
p20 = Point(11.8475618262, 20.0033935351)
p21 = Point(11.3920350935, 21.8634638891)
p22 = Point(13.9354631185, 24.3308841546)
p23 = Point(13.5938162539, 38.1105890219)
p24 = Point(24.1848690591, 38.2244708803)
p25 = Point(24.4126336356, 24.444766013)
p26 = Point(55.0034604613, 31.8185966887)
q1 = Point(3.24420510708, -8.15376887417)
q2 = Point(0.425631795717, 40.7867556291)
q3 = Point(74.7334736409, 41.0429887417)
q4 = Point(72.4273682043, -3.2853397351)
q5 = Point(46.1206855797, -3.39922159351)
q6 = Point(46.5477405272, 14.1385119205)
q7 = Point(31.1737042834, 14.1385119205)
q8 = Point(30.6612364086, -2.77287350993)
q9 = Point(36.0421490939, 63.8477357616)
q10 = Point(33.0687358636, -6.23156460647)
q11 = Point(33.0687358636, 0.942992473178)
q12 = Point(43.5459063806, 1.17075618999)
q13 = Point(43.4320240924, -6.34544646487)
q14 = Point(32.9554535754, 3.79003893336)
q15 = Point(43.5465063806, 3.90392079176)
q16 = Point(43.6603886689, 11.7617690219)
q17 = Point(33.0693358636, 11.7617690219)
q18 = Point(44.588207114, 2.92489489112)
q19 = Point(44.7951588274, 3.2565389383)
r1 = Point(-1.7977553542, 34.9218011683)
r2 = Point(35.7859151565, 61.7978708609)
r3 = Point(76.4278492586, 35.4058602649)
r4 = Point(81.1558657331, 39.0096688742)
r5 = Point(35.7859151565, 67.5177245033)
r6 = Point(-4.45935667216, 40.6709403974)
r7 = Point(-3.36406878189, 40.3237233458)
r8 = Point(35.6532284356, 66.2982614986)
r9 = Point(79.5503007008, 39.0587755778)
r10 = Point(-0.560112780041, 39.4393112026)
r11 = Point(0.409767311609, 40.7975865301)
r12 = Point(2.28890020367, 38.1236919887)
r13 = Point(-1.48508401222, 35.3824461371)
r14 = Point(74.6742679716, 40.8428379594)
r15 = Point(76.6678944991, 35.8138423262)
r16 = Point(76.3445060572, 35.6701330621)
r17 = Point(73.9198217395, 37.3404780403)
w1 = Point(15.6424781261, 25.7354125337)
w2 = Point(15.0351055189, 36.5921497019)
w3 = Point(22.7790744958, 35.8329373125)
w4 = Point(23.044796608, 26.3048218258)
w5 = Point(16.8192609644, 32.2266784629)
w6 = Point(16.8951857169, 35.9088585514)
w7 = Point(20.8810355519, 35.794976693)
w8 = Point(22.2096606344, 32.4544421798)
w9 = Point(19.3869431631, 31.5623635762)
w10 = Point(18.832149835, 26.1529938891)
w11 = Point(21.9069716172, 30.0249770749)
w12 = Point(17.0112332233, 29.2279240661)
w13 = Point(22.3639007701, 35.6053281369)
w14 = Point(15.5311634763, 33.0999472519)
w15 = Point(18.6083852585, 36.4027611457)
w16 = Point(12.4738830986, 21.6651910736)
w17 = Point(26.7301843088, 22.0110539054)
#!PYRTIST:REFPOINTS:END
# This example shows how to draw a house in a cartoon-ish way.
# I had in mind the typical house here in England...

from pyrtist.lib2d import *

# We definitely need a brick pattern to draw the walls
# Try to change to gui(bricks) to see how this is done.

# Just one brick.
brick = Window()
brick.take(
  Poly(Color(1.000, 0.496, 0.000), 0.5,
       p1, p2, p3, p4, p5, p6, p7, p8, p9, p10),
  Hot("center", p11)
)

# A brick pattern, which we save as a cairo surface.
bricks = Window()
bricks.take(Put(brick),
            Put(brick, "t", Near("center", p12)),
            BBox(bbox1, bbox2))
surface = bricks.draw(mode="rgb24", resolution=50/25.4)

# We make a proper pattern using the cairo surface.
pat = Image(surface, Scale(3.5), Extend.reflect, Filter.best)

house = Window()
house.BBox(bbox3, bbox4)

# Colors used below.
panes_color = Color(0.85, 0.95, 1)
window_color = Grey(1)
door_color = Color(0.545, 0.272, 0.078)
border = Border(Color.black, 0.5, Join.round, Cap.round)

roof_color_bright = Color(0.9, 0.35, 0.3)
roof_color = Color(0.647, 0.165, 0.165)

# Roof.
house.take(
  Poly(r1, r2, r3, r4, r5, r6, Style(roof_color, border)),
  Line(r7, r8, r9, roof_color_bright, Border(1, Cap.round)),
  Poly(r10, r11, r12, r13, roof_color.darken(0.3)),
  Poly(r14, r15, r16, r17, roof_color.darken(0.3)),
)

# Just to make the floor more regular...
q4.y = q5.y = q8.y = q1.y

# Now draw the door.
house.take(
  Poly(q8, q7, q6, q5, q5, Style(border, door_color)),
  Poly(q10, q11, q12, q13, Style(border, door_color.darken(0.6))),
  Poly(q14, q15, q16, q17, Style(border, door_color.darken(0.6))),
  Poly(q1, q2, q9, q3, q4, q5, q6, q7, q8, Style(pat, border)),
  # And the door handle.
  Circle(q18, 1),
  Circle(q19, 0.3, Grey(0.5))
)

# Create a window.
window = Window()
window_marks = Border(Color.black, 0.3, Cap.round)
window.take(
  Poly(p14, p15, p16, p17, Style(border, window_color)),
  Poly(p14, p17, p18, p19, p20, p21,
       Style(border, window_color.darken(0.8))),
  Poly(p22, p23, p24, p25, Style(panes_color.darken(0.9), border)),
  Poly(0.4, w1, w2, w3, w4, panes_color.darken(0.95)),
  Poly(0.4, w5, w6, w7, w8, panes_color),
  Hot("center", w9)
)
window.take(Line(window_marks, *ab)
            for ab in [(w10, w11), (w12, w13), (w14, w15), (w16, w17)])

# And draw it twice.
house.take(
  Put(window),
  Put(window, "t", Near("center", p26))
)

#house.save('house.png')

gui([house, bricks][0])
