from .core_types import Point

def intersection(segment1, segment2):
    '''Find the intersection point between the two segment identified by the
    two pairs of points segment1 and segment2.
    '''
    a1 = segment1[0]
    b1 = segment2[0]
    a12 = segment1[1] - a1
    b12 = segment2[1] - b1
    ob12 = Point(b12.y, -b12.x)
    denom = ob12.dot(a12)
    if denom == 0.0:
        raise ValueError("Intersection not found: segments are parallel")
    return a1 + a12 * (ob12.dot(b1 - a1)/denom)
