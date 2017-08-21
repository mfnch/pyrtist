# Some symbols used in electronics
import math
from pyrtist.lib2d import *
from pyrtist.lib2d import arrows

def build_diode(a=2.0, b=3.0, d1=4.0, s=0.4):
    p1 = Point(-d1, 0)
    p2 = Point(b + 0.5*s + d1, 0)
    return Window(
        Hot('A', p1),
        Hot('K', p2),
        Poly((0, -a), (0, a), (b, 0)),
        Poly(Point(b - s/2, -a), Point(b + s/2, -a),
             Point(b + s/2,  a), Point(b - s/2,  a)),
        Line(0.4, p1, p2)
    )

# Intro: schematic symbol of a diode.
# This is a {type(Figure)} with two hot points ``"A"`` and ``"K"``
# for the two terminals.
# Example: ``Put[diode, "tr", Near["A", point1], Near["K", point2]]``
# Use: | Put[(**boxer-self*), "rt", Near["A", (**boxer-new-point*)],
#            Near["K", (**boxer-new-point*)]]
# Preview: |include "electric"
#          |include "guitools"
#          |fg = Figure[diode, BBox[(-6, -3), (8, 2.5)],
#                       GuiShowHotPoints[diode]]
#          |(**view:fg*)
diode = build_diode()

def build_resistor(d1=1.0, v1=Point(1, 4), v2=Point(1, -4)):
    ret = Window()
    dps = [Point(d1, 0), v1/2] + [v2, v1]*5 + [v2, v1/2, Point(d1, 0)]
    p0 = p = Point()
    line = Line(0.4, p0)
    for dp in dps:
        p += dp
        line << p
    ret << Args(line, Hot(0, p0), Hot(1, p))
    return ret

# Intro: schematic symbol of a resistor.
# This is a {type(Figure)} with two hot points for the two terminals.
# Use: | Put[(**boxer-self*), "rt", Near[0, (**boxer-new-point*)],
#            Near[1, (**boxer-new-point*)]]
# Preview: |include "electric"
#          |include "guitools"
#          |fg = Figure[resistor, BBox[(-3, -3), (15, 3)],
#                       GuiShowHotPoints[resistor]]
#          |(**view:fg*)
resistor = build_resistor()

def build_inductance(a=1.1, b=2.2, d1=2.0, v=0.5):
    ret = Window()
    v1 = Point(v + a, b)
    v2 = Point(v + a, -b)
    v3 = Point(v - a, -b)
    v4 = Point(v - a, b)
    dps = [Point(d1, 0)] + [v1, v2, v3, v4]*6 + [v1, v2, Point(d1, 0)]
    p0 = p = Point()
    line = Line(0.4, p0)
    for dp in dps:
        p += dp
        line << p
    ret << Args(line, Hot(0, p0), Hot(1, p))
    return ret

# Intro: schematic symbol of an inductance.
# This is a {type(Figure)} with two hot points for the two terminals.
# Use: | Put[(**boxer-self*), "rt", Near[0, (**boxer-new-point*)],
#            Near[1, (**boxer-new-point*)]]
# Preview: |include "electric"
#          |include "guitools"
#          |fg = Figure[inductance, BBox[(-3, -3), (20, 3)],
#                       GuiShowHotPoints[inductance]]
#          |(**view:fg*)
inductance = build_inductance()

def build_capacitor(h=4.0, d=1.0):
    return Window(
        Line(1.2, Point(-h, -d), Point(h, -d)),
        Line(1.2, Point(-h, d), Point(h, d)),
        Hot(0, (0.0, -d)), Hot(1, (0.0, d))
    )

# Intro: schematic symbol of a capacitor.
# This is a {type(Figure)} with two hot points for the two terminals.
# Use: | Put[capacitor, "rt", Near[0, (**boxer-new-point*)],
#            Near[1, (**boxer-new-point*)]]
# Preview: |include "electric"
#          |include "guitools"
#          |fg = Figure[capacitor, BBox[(-5, -3), (5, 3)],
#                       GuiShowHotPoints[capacitor]]
#          |(**view:fg*)
capacitor = build_capacitor()

def build_transistor(transistor_type, r=5.0, d=0.8):
    hd = 0.5*d
    ld = r*0.4
    ll = r*0.5
    lp = r*0.2
    a = -0.65*math.pi
    base = Point(r - hd, 0)
    collector = (r - hd)*Point(math.cos(a), math.sin(a))
    emitter = (r - hd)*Point(math.cos(-a), math.sin(-a))

    ret = Window()
    ret << Args(Circles(Point(0, 0), r, r - hd),
                Line(d, Point(ld, -ll), Point(ld, ll)),
                Line(d, hd, base, (ld, 0)),
                Hot("C", collector), Hot("E", emitter), Hot("B", base))
    style = Args(hd, Cap.round)
    if transistor_type == 'NPN':
        ret << Line(style, collector, arrows.triangle, Point(ld - hd, -lp))
        ret << Line(style, Point(ld, lp), emitter)
    else:
        ret << Line(style, (ld, -lp), collector)
        ret << Line(style, (ld, lp), arrows.triangle, emitter)
    return ret

# Intro: schematic symbol of an NPN transistor.
# This is a {type(Figure)} with three hot points ``"B"``, ``"C"``, ``"E"``
# for the base, collector and emitter terminals respectively.
# Use: | Put[transistor_NPN, "rt", Near["C", (**boxer-new-point*)],
#            Near["E", (**boxer-new-point*)], Near["B", (**boxer-new-point*)]]
# Preview: |include "electric"
#          |include "guitools"
#          |fg = Figure[transistor_NPN, BBox[(-6, -7.5), (6, 6)],
#                       GuiShowHotPoints[transistor_NPN]]
#          |(**view:fg*)
transistor_NPN = build_transistor('NPN')

# Intro: schematic symbol of a PNP transistor.
# This is a {type(Figure)} with three hot points ``"B"``, ``"C"``, ``"E"``
# for the base, collector and emitter terminals respectively.
# Use: | Put[transistor_PNP, "rt", Near["C", (**boxer-new-point*)],
#            Near["E", (**boxer-new-point*)], Near["B", (**boxer-new-point*)]]
# Preview: |include "electric"
#          |include "guitools"
#          |fg = Figure[transistor_PNP, BBox[(-6, -7.5), (6, 6)],
#                       GuiShowHotPoints[transistor_PNP]]
#          |(**view:fg*)
transistor_PNP = build_transistor('PNP')
