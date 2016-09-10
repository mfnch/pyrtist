from .base import Taker, combination
from .core_types import Point

class BBox(Taker):
    def __init__(self, *args):
        self.min_point = None
        self.max_point = None
        super(BBox, self).__init__(*args)

    def __str__(self):
        return 'BBox({}, {})'.format(self.min_point, self.max_point)

    def __nonzero__(self):
        return (self.min_point is not None)

    def __iter__(self):
        return iter(filter(None, (self.min_point, self.max_point)))

@combination(tuple, BBox)
@combination(Point, BBox)
def fn(point, bbox):
    point = Point(point)
    if bbox.min_point is None:
        bbox.min_point = bbox.max_point = point
    else:
        bbox.min_point = Point(x=min(bbox.min_point.x, point.x),
                               y=min(bbox.min_point.y, point.y))
        bbox.max_point = Point(x=max(bbox.max_point.x, point.x),
                               y=max(bbox.max_point.y, point.y))
