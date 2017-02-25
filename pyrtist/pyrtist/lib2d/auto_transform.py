# Copyright (C) 2017 by Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Pyrtist.
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

import math
import copy
from collections import OrderedDict as odict

from .core_types import *
psum = Point.sum


class Constraint(object):
    def __init__(self, src, dst, weight=1.0):
        self.src = src
        self.dst = dst
        self.weight = weight

    def __repr__(self):
        return ('Constraint({}, {}, {})'
                .format(repr(self.src), repr(self.dst), repr(self.weight)))


class AutoTransform(object):
    var_mapping = odict((('tx', 'auto_translate_x'),
                         ('ty', 'auto_translate_y'),
                         ('r', 'auto_rotate'),
                         ('s', 'auto_scale'),
                         ('i', 'auto_invert'),
                         ('a', 'auto_aspect')))

    def __init__(self, auto_translate_x=False, auto_translate_y=False,
                 auto_rotate=False, auto_scale=False, auto_invert=False,
                 auto_aspect=False):
        self.auto_translate_x = auto_translate_x
        self.auto_translate_y = auto_translate_y
        self.auto_rotate = auto_rotate
        self.auto_scale = auto_scale
        self.auto_invert = auto_invert
        self.auto_aspect = auto_aspect

    @staticmethod
    def from_string(s):
        tokens = []
        for i, c in enumerate(s.lower()):
            if c in 'xy' and len(tokens) > 0:
                if tokens[-1][-1] == 't':
                    tokens[-1] += c
                    continue

            if c in '+-':
                tokens.append(c)
            elif c in 'trsai':
                if len(tokens) > 0 and tokens[-1] in '+-':
                    tokens[-1] += c
                else:
                    tokens.append('+' + c)
            else:
                raise ValueError('Invalid character \'{}\' at position {}'
                                 .format(c, i))

        kwargs = {}
        for token in tokens:
            do_set = token.startswith('+')
            var_name = AutoTransform.var_mapping.get(token[1:])
            if var_name is None:
                assert token[1:] == 't'
                kwargs['auto_translate_x'] = \
                  kwargs['auto_translate_y'] = do_set
            else:
                kwargs[var_name] = do_set

        return AutoTransform(**kwargs)

    def __repr__(self):
        return 'AutoTransform.from_string({})'.format(repr(self.get_string()))

    def is_auto(self):
        'Whether any auto transformation is set.'

        for full in AutoTransform.var_mapping.itervalues():
            if getattr(self, full):
                return True
        return False

    def get_string(self):
        '''Obtain the string that can be given to AutoTransform.from_string()
        to obtain this AutoTransform object.
        '''

        s = ''
        for short, full in AutoTransform.var_mapping.iteritems():
            if getattr(self, full):
                s += short
        return s

    def copy(self):
        return copy.deepcopy(self)

    def calculate(self, transform, constraints):
        weight_sum = sum(c.weight for c in constraints)
        transformations_left = self.copy()
        auto_translate = self.auto_translate_x or self.auto_translate_y
        if auto_translate:
            if weight_sum == 0.0:
                raise ValueError('The sum of all weights is zero')

            # One or both of the translational degrees of freedom have to be
            # handled automatically: we automatically determine Q and T.
            src_avg = psum((c.src*c.weight for c in constraints))/weight_sum
            dst_avg = psum((c.dst*c.weight for c in constraints))/weight_sum

            transform.rotation_center = src_avg
            if self.auto_translate_x == self.auto_translate_y:
                # Auto-translate along both x and y.
                transform.translation = dst_avg - src_avg
            elif self.auto_translate_x:
                # Auto-translate only along x.
                transform.translation.x -= src_avg.x
                transform.translation.y = dst_avg.y - src_avg.y
            else:
                # Auto-translate only along y.
                assert self.auto_translate_y
                transform.translation.x = dst_avg.x - src_avg.x
                transform.translation.y -= src_avg.y

        # Exit if translating was the only thing we had to do.
        transformation_left = self.copy()
        transformation_left.auto_translate_x = False
        transformation_left.auto_translate_y = False
        if not transformation_left.is_auto():
            return transform

        # Compute all the averages we need in the next steps.
        U = transform.translation + transform.rotation_center

        gs = [c.src - transform.rotation_center
              for c in constraints]
        wgs = [(c.src - transform.rotation_center)*c.weight
              for c in constraints]
        ss = [c.dst - U for c in constraints]

        g2_avg = psum(Point(x=wg.x*gs[i].x,
                            y=wg.y*gs[i].y)
                      for i, wg in enumerate(wgs))/weight_sum
        i_avg  = psum(Point(x=wg.x*ss[i].x,
                            y=wg.y*ss[i].y)
                      for i, wg in enumerate(wgs))/weight_sum
        j_avg  = psum(Point(x=wg.x*ss[i].y,
                            y=-wg.y*ss[i].x)
                      for i, wg in enumerate(wgs))/weight_sum

        # We now have three different cases to deal with.
        if not transformation_left.auto_aspect:
            # CASE 1: Proportions are manually fixed.
            aspect = transform.scale_factors.normalized()
            AB = Point(x=i_avg.dot(aspect),
                       y=j_avg.dot(aspect))

            if self.auto_rotate:
                # Find the rotation angle, if automatic rotation is selected.
                transform.rotation_angle = math.atan2(AB.y, AB.x)

            if self.auto_scale:
                c = aspect.x*aspect.x*g2_avg.x + aspect.y*aspect.y*g2_avg.y
                rotation_vector = Pang(transform.rotation_angle)
                scale_factor = (rotation_vector*AB)/c
                transform.scale_factors = aspect*scale_factor
        else:
            raise NotImplementedError('Case not yet implemented')
        return transform
