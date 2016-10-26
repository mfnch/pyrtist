__all__ = ('Transform',)

import math

from .core_types import *


class Transform(object):
    '''Parametrized `Matrix` object.
    A `Transform` object represents a transformation of the two dimensional
    coordinate system, parametrized by: a translation vector, a rotation angle
    and rotation center and scaling factors along each dimension.
    '''

    def __init__(self, translation=None, rotation_center=None,
                 scale_factors=None, rotation_angle=None):
        self.translation = translation or Point()
        self.rotation_center = rotation_center or Point()
        self.scale_factors = scale_factors or Point(1.0, 1.0)
        self.rotation_angle = rotation_angle or 0.0

    @staticmethod
    def create_translation(point):
        '''Return a new translation transformation.'''
        return Transform(translation=point)

    @staticmethod
    def from_matrix(mx):
        '''Build a `Transform` object from a given `Matrix`, assuming (0, 0)
        as a rotation center.
        '''

        determinant = mx.det()
        if determinant == 0.0:
            raise ValueError('The matrix is singular (determinant==0.0)')

        (m11, m12, m13), (m21, m22, m23) = mx.value
        scale_x = math.sqrt(m11*m11 + m21*m21)
        return Trasform(translation=Point(x=m13, y=m23),
                        scale_factors=Point(x=scale_x, y=determinant/scale_x),
                        rotation_angle=math.atan2(m21, m11))

    def get_matrix(self):
        'Get the `Matrix` corresponding to this `Transformation`.'

        rcos = math.cos(self.rotation_angle)
        rsin = math.sin(self.rotation_angle)
        rcenter = self.rotation_center
        scale = self.scale_factors
        translation = self.translation

        m11 = scale.x*rcos; m12 = -scale.y*rsin
        m21 = scale.x*rsin; m22 = scale.y*rcos
        m13 = (1.0 - m11)*rcenter.x - m12*rcenter.y + translation.x
        m23 = (1.0 - m22)*rcenter.y - m21*rcenter.x + translation.y
        return Matrix([[m11, m12, m13],
                       [m21, m22, m23]])
