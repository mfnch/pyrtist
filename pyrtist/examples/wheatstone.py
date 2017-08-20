#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(-22.6152212121, 21.856384472)
bbox2 = Point(22.357695082, -21.4080237288)
p1 = Point(-0.255807878788, -19.5900623377)
p2 = Point(19.7791411043, -0.0303101075269)
p3 = Point(0.127607361963, 19.7838873118)
p4 = Point(-20.5447852761, 0.223718064516)
#!PYRTIST:REFPOINTS:END
from pyrtist.lib2d import *

from pyrtist.lib2d.prefabs.electric import *

# This is not a real Wheatstone bridge.
# We replace some of the resistors with other components, just to make
# the example more interesting!
w = Window()

d1 = 20.0  # d1 = half diagonal
w << BBox(bbox1, bbox2)

# Places the four components.
# Each component has two connections. Here we want to rotate and translate
# ("rt") a resistor such that the first of its terminals is near to
# the point p1 and the second one is near to p2.
r12 = Put(capacitor, "rt", Near(0, p1), Near(1, p2)) >> w
# We proceed in a similar way for the other components:
r23 = Put(diode, "rt", Near("A", p2), Near("K", p3)) >> w
r34 = Put(inductance, "rt", Near(0, p3), Near(1, p4)) >> w
r41 = Put(resistor, "rt", Near(0, p4), Near(1, p1)) >> w

# Connect the four components
for lhs, rhs in [(p1, r12[0]), (r12[1], p2),
                 (p2, r23[0]), (r23[1], p3),
                 (p3, r34[0]), (r34[1], p4),
                 (p4, r41[0]), (r41[1], p1)]:
    w << Line(0.4, lhs, rhs)

# Drawing small circles on the nodes
# Here you see how a single Circle instruction can be used to draw many
# circles. To start a new circle one should use the separator ";" and
# specify only the quantities that changed with respect to the previous
# circle. Here we change only the center, so we need to specify the radius
# only once!
w << Circles(0.8, p1, p2, p3, p4)

#w.save('wheatstone.png')
gui(w)
