#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(-37.5826573705, 64.8614862385)
bbox2 = Point(146.843632129, -72.3863575296)
bg1 = Point(54.63065, -25.8746116667)
bg2 = Point(132.618249036, -25.8746116667)
fg1 = Point(25.9196635922, -64.5110530769)
fg2 = Point(127.536394984, -65.2736212928)
gui1 = Point(-35.0423175368, -17.7413972927)
gui2 = Point(-3.16141042462, -15.7586425439)
gui3 = Point(17.4404749672, -9.35403111111)
gui4 = Point(18.3549837973, -3.50828722222)
gui5 = Point(17.7961162534, 3.099945)
#!PYRTIST:REFPOINTS:END
from pyrtist.lib2d import *
from pyrtist.lib2d.geom import intersection
from pyrtist.lib2d.tools import Bar, Sphere

w = Window()
w.BBox(bbox1, bbox2)

# Colors of the planes.
c1 = Color.yellow
c2 = Color.red
c3 = Color.green
c4 = Color.blue

# Some controls to let you see through the planes.
plane1 = Bar(pos=gui1, cursor=gui2, fg_color=c1)
plane2 = Bar(cursor=gui3, fg_color=c2)
plane3 = Bar(cursor=gui4, fg_color=c3)
plane4 = Bar(cursor=gui5, fg_color=c4)
w.take(plane1, plane2, plane3, plane4)

c1.a = plane1.value
c2.a = plane2.value
c3.a = plane3.value
c4.a = plane4.value

fg2.y = fg1.y
lx = fg2 - fg1
ly = Point(0, lx.x)
fg3 = fg1 + ly
fg4 = fg2 + ly

bg2.y = bg1.y
lx = bg2 - bg1
ly = Point(0, lx.x)
bg3 = bg1 + ly
bg4 = bg2 + ly

center = intersection((fg1, bg4), (fg3, bg2))
down = intersection((fg1, bg2), (bg1, fg2))
up = intersection((fg3, bg4), (bg3, fg4))
front = intersection((fg1, fg4), (fg2, fg3))
rear = intersection((bg1, bg4), (bg2, bg3))
sph = Sphere(Color.blue, 2)
w.take(Sphere(sph, bg1), Sphere(sph, bg2), Sphere(sph, bg3), Sphere(sph, bg4))

s1 = Style(Border(Color.black, 0.2, Join.round))
w.take(
  Poly(c4, s1, bg2, rear, center),
  Poly(c3, s1, bg4, rear, center),
  Poly(c3, s1, bg1, rear, center),
  Poly(c1, s1, bg2, down, center),
  Poly(c4, s1, fg2, center, bg2),
  Poly(c1, s1, bg2, center, bg4),
  Poly(c4, s1, bg3, rear, center),
  Poly(c2, s1, bg1, center, bg3),
  Poly(c2, s1, fg2, center, fg4),
  Poly(c2, s1, bg1, down, center),
  Poly(c2, s1, fg2, down, center),
  Poly(c1, s1, bg4, up, center),
  Poly(c3, s1, fg1, center, bg1),
  Poly(c3, s1, fg4, center, bg4),
  Poly(c2, s1, bg3, up, center),
  Poly(c4, s1, fg3, center, bg3),
  Poly(c2, s1, fg4, up, center),
  Poly(c1, s1, fg1, center, fg3),
  Poly(c1, s1, fg1, down, center),
  Poly(c1, s1, fg3, up, center),
  Poly(c4, s1, fg2, front, center),
  Poly(c3, s1, fg1, front, center),
  Poly(c3, s1, fg4, front, center),
  Poly(c4, s1, fg3, front, center)
)

sph = Sphere(sph, 3)
w.take(Sphere(sph, fg1), Sphere(sph, fg2), Sphere(sph, fg3), Sphere(sph, fg4))

gui(w)
