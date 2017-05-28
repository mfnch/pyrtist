# Copyright (C) 2017 Matteo Franchin
#
# This file is part of Pyrtist.
#   Pyrtist is free software: you can redistribute it and/or modify it
#   under the terms of the GNU Lesser General Public License as published
#   by the Free Software Foundation, either version 2.1 of the License, or
#   (at your option) any later version.
#
#   Pyrtist is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with Pyrtist.  If not, see <http://www.gnu.org/licenses/>.

'''Infrastructure helpers for the library.'''

__all__ = ('getClassName', 'create_enum', 'alias', 'combination',
           'RejectError', 'Taker', 'Args')

import types

def getClassName(obj):
    return obj.__class__.__name__
get_class_name = getClassName

class Enum(object):
    def __init__(self, name, value=None):
        self.name = name
        self.value = value

    def __str__(self):
        return '{}.{}'.format(get_class_name(self), self.name)

    def __repr__(self):
        args = ((self.name,) if self.value is None
                else (self.name, self.value))
        return '{}({})'.format(get_class_name(self),
                               ', '.join(map(repr, args)))

def create_enum(name, doc, *enums):
    d = ({'__doc__': doc} if doc is not None else {})
    new_class = type(name, (Enum,), d)
    for name in enums:
        setattr(new_class, name, new_class(name))
    return new_class

def alias(name, target, **attrs):
    return type(name, (target,), attrs)


class RejectError(Exception):
    pass


class Taker(object):
    '''Base class for taker objects.

    The Taker base class gives to inherited classes the ability of "taking"
    other objects via the take() method. Every argument, `x`, given to
    Taker.take() is handled by calling a separate function which was provided
    via the `combination` annotation and is identified by looking at the type
    of `x`. Let's see an example:

      class Line(Taker):
          def __init__(self, *args):
              super(self, Line).__init__()
              self.points = []
              self.color = None
              self.take(*args)

      @combination(Point, Taker)
      def point_at_line(point, line):
          line.points.append(point)

      @combination(Color, Taker)
      def color_at_line(color, line):
          assert line.color is None, 'Color should be given only once'
          line.color = color

    The code above defines a new taker object `Line` which can be given `Point`
    objects. Every `Point` object given to `Line.take` is handled by calling
    the function `point_at_line`. Similarly, `Color` objects can also be given
    to `Line.take` and are handled by calling the function `color_at_line`.
    The user can now do:

      line = Line(Point(p1), Point(p2), Color(c), Point(p3))

    Which ends up being equivalent to:

      line = Line()
      point_at_line(Point(p1), line)
      point_at_line(Point(p2), line)
      color_at_line(Color(c), line)
      point_at_line(Point(p3), line)

    `Taker` also overrides the operators << and >>, providing an alternative
    to using the Taker.take method. For example:

      line << Point(p1) << Point(p2) << Color(c) << Point(p3)
      # equivalent to: line.take(Point(p2), Color(c), Point(p3))

      line = Line(Point(p1), Color(c), Point(p3)) >> window1 >> window2
      # Equivalent to:
      #   line = Line(Point(p1), Color(c), Point(p3))
      #   window1.take(line)
      #   window2.take(line)

    Additionally arguments can be grouped using the `Args` object:

      args = Args(p1, c, p3)
      line1 = Line(args, p2)
      line2 = Line(args, p4)
      # Equivalent to:
      #   line1 = Line(p1, c, p3, p2)
      #   line2 = Line(p1, c, p3, p4)
    '''

    def __init__(self, *args):
        self.take(*args)

    def _take_one(self, arg):
        for arg_type in type(arg).mro():
            for ancestor_type in type(self).mro():
                combinations = getattr(ancestor_type, 'combinations', None)
                if combinations is None:
                    continue

                combination = combinations.get(arg_type)
                if combination is not None:
                    return combination(arg, self)

                if isinstance(arg, types.GeneratorType):
                    ret = None
                    for item in arg:
                        ret = self._take_one(item)
                    return ret

        raise RejectError('{} doesn\'t take {}'
                          .format(getClassName(self), getClassName(arg)))

    def take(self, *args):
        ret = None
        for arg in args:
            ret = self._take_one(arg)
        return ret

    def __lshift__(self, arg):
        self._take_one(arg)
        return self

    def __rshift__(self, parent):
        parent._take_one(self)
        return self


def create_method(child, parent, fn):
    def new_method(p, *args, **kwargs):
        new_obj = child(*args, **kwargs)
        fn(new_obj, p)
        return new_obj
    return new_method

def combination(child, parent, method_name=None):
    assert issubclass(parent, Taker), \
      '{} is not a Taker: cannot add combination'.format(parent)
    combinations = parent.__dict__.get('combinations')
    if combinations is None:
        combinations = parent.combinations = {}
    def combination_adder(fn):
        combinations[child] = fn
        if method_name is not None:
            setattr(parent, method_name, create_method(child, parent, fn))
        return fn
    return combination_adder


class Args(tuple):
    '''Tuple object which collects one or more arguments to be passed to a
    Taker object. The example below:

      taker << Args(one, two, three)

    is equivalent to any of the following:

      taker.take(one, two, three)
      taker << one << two << three
    '''
    def __new__(cls, *args):
        return tuple.__new__(cls, args)


@combination(Args, Taker)
def args_at_taker(args, taker):
    '''Give all the members of `Args` to the `Taker` in the same order they
    were originally provided when constructing the `Args` object.
    '''
    for arg in args:
        taker._take_one(arg)
