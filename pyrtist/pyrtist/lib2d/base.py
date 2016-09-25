'''Infrastructure helpers for the library.'''

__all__ = ('getClassName', 'enum', 'alias', 'combination',
           'RejectError', 'Taker')

import types

def getClassName(obj):
    return obj.__class__.__name__

def enum(name, doc, **enums):
    new_class = type(name, (int,), {'__doc__': doc})
    for key, val in enums.iteritems():
        setattr(new_class, key, new_class(val))
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
