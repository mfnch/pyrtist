#!PYRTIST:VERSION:0:0:1
from pyrtist.lib2d import Point, Tri
#!PYRTIST:REFPOINTS:BEGIN
bbox1 = Point(0.0, 50.0); bbox2 = Point(100.0, 0.0)
p1 = Point(13.2653061224, 15.5612119398)
p2 = Point(23.8095238095, 38.6904453846)
p3 = Point(38.7755102041, 18.2822982274)
p4 = Point(41.156462585, 7.22788518395); p5 = Point(75.0, 39.2006490635)
p6 = Point(88.7755102041, 16.5816192977)
p7 = Point(66.156462585, 19.3027055853)
#!PYRTIST:REFPOINTS:END
from pyrtist.lib2d import *

w = Window()          # Create a new Window and set the
w.BBox(bbox1, bbox2)  # bounding box from p1 and p2.

# The line below defines a Poly object, representing a polygon.
# The new object is passed directly to the Window object which
# just draws it.
# p1, p2, p3 are three ``reference'' points which are shown
# in the GUI as small white squares. They can be selected
# (they become yellow) with a left mouse click (keep the SHIFT
# key pressed to select multiple points). Once a point is yellow
# it can be moved with a drag-and-drop action. 
w.take(Poly(p1, p2, p3))

# The line below creates a new 'Poly' object and - rather
# than passing it to the Window - assigns it to a new variable.
# No drawing is performed at this stage.
another_polygon = Poly(p4, p5, p6)

# We now create a new Window and draw the second polygon in there.
new_figure = Window()
new_figure.take(another_polygon)

# We finally put the new figure inside the first, scaling it down
# a bit. Try to change the AngleDeg[0] to AngleDeg[20], then
# press CTRL+RETURN (or try menu Run --> Execute)
w.take(Put(new_figure, Scale(0.8), AngleDeg(0), Center(p7)))

gui(w)
