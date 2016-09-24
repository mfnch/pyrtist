#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import *
#!PYRTIST:REFPOINTS:BEGIN
gui4 = Point(17.7961162534, -3.50828722222)
bbox2 = Point(146.843632129, -72.3863575296)
bbox1 = Point(-37.5826573705, 64.8614862385)
gui5 = Point(17.7961162534, 3.099945)
gui3 = Point(18.5582100551, -9.35403111111)
fg1 = Point(25.9196635922, -64.5110530769)
fg2 = Point(127.536394984, -65.2736212928)
gui2 = Point(20.5904601928, -15.199775); bg1 = Point(54.63065, -25.8746116667)
bg2 = Point(132.618249036, -25.8746116667)
gui1 = Point(-35.0423175368, -17.7413972927)
#!PYRTIST:REFPOINTS:END
from pyrtist.lib2d.geom import intersection

w = Window()

w.BBox(bbox1, bbox2)

# Colors of the planes
c1 = Color((1, 1, 0))
c2 = Color((1, 0, 0))
c3 = Color((0, 1, 0))
c4 = Color((0, 0, 1))

'''
  // Some controls to let you have some fun!
  bar_defaults.pos = gui1
  (plane1 = GUIReal[Interval[0, 1], gui2, c1])
  (plane2 = GUIReal[Interval[0, 1], gui3, c2])
  (plane3 = GUIReal[Interval[0, 1], gui4, c3])
  (plane4 = GUIReal[Interval[0, 1], gui5, c4])
  c1.a = plane1.value,  c2.a = plane2.value
  c3.a = plane3.value, c4.a = plane4.value
'''

s1 = Style(Border(Color.black, 0.2))

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
'''
  sph = Sphere[color.blue, 2]
  Sphere[sph, bg1], Sphere[sph, bg2], Sphere[sph, bg3], Sphere[sph, bg4]
'''

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

'''
  sph = Sphere[sph, 3]
  Sphere[sph, fg1], Sphere[sph, fg2], Sphere[sph, fg3], Sphere[sph, fg4]
]
'''

gui(w)
