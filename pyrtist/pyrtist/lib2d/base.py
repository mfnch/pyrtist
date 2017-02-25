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

'''Infrastructure helpers for the library.'''

__all__ = ('getClassName', 'create_enum', 'alias', 'combination',
           'RejectError', 'Taker')

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

    def __call__(self, *args):
        return self.take(*args)


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
