from pyter2d import *

w = Window()
l = w.Line((1, 2), (1, 4), (5, 4), Close())
w.Circle((1, 2.5), 5.0)
w.Save("test.png", resolution=100)
