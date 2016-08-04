import numbers

from base import Taker, combination, RejectException

class Point(Taker):
    def __init__(self, value=None, x=0.0, y=0.0):
        self.x = x
        self.y = y
        super(Point, self).__init__()
        if value is not None:
            self.take(value)

    def __repr__(self):
        return 'Point(({x}, {y}))'.format(x=self.x, y=self.y)

    def __add__(self, value):
        return Point(x=self.x + value.x, y=self.y + value.y)

    def __sub__(self, value):
        return Point(x=self.x - value.x, y=self.y - value.y)

    def __mul__(self, value):
        if isinstance(value, numbers.Number):
            return Point(x=self.x*value, y=self.y*value)
        else:
            return float(self.x*value.x + self.y*value.y)

def Px(value):
    return Point(x=value)

def Py(value):
    return Point(y=value)

@combination(Point, Point)
def fn(child, parent):
    parent.x = child.x
    parent.y = child.y

@combination(tuple, Point)
def fn(tp, point):
    if len(tp) != 2:
        raise RejectException()
    x, y = tp
    point.x = float(x)
    point.y = float(y)


class PointTaker(Taker):
    def __init__(self, *args):
        self.points = []
        super(PointTaker, self).__init__(*args)

    def __iter__(self):
        return iter(self.points)

    def __len__(self):
        return len(self.points)

@combination(Point, PointTaker)
def fn(point, point_taker):
    point_taker.points.append(point)

@combination(tuple, PointTaker)
def fn(tp, point_taker):
    if len(tp) != 2:
        raise RejectException()
    point_taker.take(Point(tp))
