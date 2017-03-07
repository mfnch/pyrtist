#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(30.5430530612, 47.9487259669)
bbox2 = Point(91.596572, 2.55635137931)
gradient_1 = Point(80.7540587224, 20.1550310997)
gradient_2 = Point(85.6947292383, 35.1338943299)
gui1 = Point(62.9776352697, 13.1407282123)
gui2 = Point(65.2669775934, 3.96126117318)
gui3 = Point(89.4384517766, 30.4841370558)
gui4 = Point(89.4384517766, 37.8813451777)
gui5 = Point(73.1541878173, 35.5856598985)
gui6 = Point(34.9967846473, 16.4555357542)
gui7 = Point(33.7249278008, 24.3600768156)
gui8 = Point(41.6034263959, 43.7480964467)
gui9 = Point(52.417302799, 46.9387755102)
normal_1 = Point(51.8444233207, 32.5715189873)
normal_2 = Point(45.9996831432, 39.95)
ruler_1 = Point(49.2415813278, 22.0652100559)
ruler_2 = Point(61.4514070539, 25.3800175978)
ruler_3 = Point(68.1080481622, 34.6069620253)
rulerx_1 = Point(45.934753527, 25.6350027933)
rulerx_2 = Point(55.6008655602, 21.5552396648)
rulery_1 = Point(58.0112896806, 22.3508853952)
rulery_2 = Point(62.1677267813, 30.2716455326)
rulerz_1 = Point(61.5009505703, 31.5537974684)
rulerz_2 = Point(59.9762357414, 35.8791139241)
v1 = Point(55.597715736, 25.1275380711)
v2 = Point(56.8699238579, 30.2290609137)
v3 = Point(59.7221166033, 33.0803797468)
v4 = Point(54.8343908629, 34.5653553299)
v5 = Point(50.3197084918, 36.6424050633)
v6 = Point(46.7620405577, 34.3525316456)
v7 = Point(45.9289340102, 29.2087563452)
v8 = Point(50.5088832487, 27.1681472081)
#!PYRTIST:REFPOINTS:END
from pyrtist.lib2d import *

w = Window()
w.BBox(bbox1, bbox2)

c = Color(0.3, 0.7, 1)
c2 = Color(1.0, 1.0, 0.0, 0.8)
c3 = Color(0.7, 1, 1)

g = Gradient(Line(gradient_1, gradient_2), c, 0.7, c3, c)
ss = StrokeStyle(Border(Color.black, 0.25, Join.round))
s1 = Style(ss)
s2 = Style(Border(ss, Dash(0.4)))
s3 = Border(Color(0, 0, 0, 0.5), 0.25, Dash(0.4))

w.take(
  Poly(s1, gui2, gui3, gui4, 0.5, gui5, gui1, 0, c.darken(0.6)),
  Poly(s1, gui1, gui2, gui6, gui7, c.darken(0.85)),
  Poly(s1, gui7, 0.5, gui8, 0.5, 0, gui9, 0.5, 0, gui4, gui5, 0.5, 0, gui1, g)
)

ps = Line(v1, v2, v3, v4, v5, v6, v7, v8)
ps2 = Line(p + Point(-0.7, 2) for p in ps)
ps3 = Line(p - Point(-0.7, 2) for p in ps)

w.take(
  Color(1, 1, 0, 0.5),
  Poly(s2, 1, *ps3),
  Poly(1, ps[0], ps[1], ps[2], 0, ps3[2], 0, 1, ps3[1], ps3[0]),
  Poly(1, ps[6], ps[7], ps[0], 0, ps3[0], 0, 1, ps3[7], ps3[6]),
  Line(s3, ps[6], ps3[6]),
  Line(s3, ps[0], ps3[0]),
  Line(s3, ps[2], ps3[2]),

  Poly(c2, 1, *ps),
  Poly(s1, 1, *ps2),
  Poly(s1, 0, 1, ps[1], ps[2], 0, ps2[2], 0, 1, ps2[1], ps2[0], ps[0]),
  Poly(s1, 1, ps[6], ps[7], ps[0], 0, ps2[0], 0, 1, ps2[7], ps2[6]),

  StrokeStyle(Color.black),
  Line(0.3, normal_1, normal_2, arrows.normal),
  Circle(normal_1, 0.5),
  Line(0.2, arrows.ruler, rulery_1, arrows.ruler, rulery_2),
  Line(0.2, arrows.ruler, rulerz_1, arrows.ruler, rulerz_2),
  Line(0.2, arrows.ruler, rulerx_1, arrows.ruler, rulerx_2),

  Texts(ruler_1, Font("sans", 2.1), "dx", ruler_2, "dy", ruler_3, "dz << dx, dy",
        normal_2, Font("sans", 4), "n", Offset(1, -0.2))
)

# Uncomment the following line to obtain a proper PNG figure
#w.save("surf_charges.png", mode='rgb24', resolution=10)

gui(w)
