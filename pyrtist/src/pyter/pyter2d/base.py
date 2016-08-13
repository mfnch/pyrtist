'''Infrastructure helpers for the library.'''

__all__ = ('getClassName', 'enum', 'alias', 'combination',
           'RejectException', 'Taker')

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


class RejectException(Exception):
    pass


class Taker(object):
    def __init__(self, *args):
        self.take(*args)

    def take(self, *args):
        combinations = self.combinations or {}
        ret = None
        for arg in args:
            arg_type = type(arg)
            combination = combinations.get(arg_type)
            if combination is not None:
                ret = combination(arg, self)
                continue

            if isinstance(arg, types.GeneratorType):
                for item in arg:
                    ret = self.take(item)
                continue

            raise ValueError('{} doesn\'t take {}'
                             .format(getClassName(self),
                                     getClassName(arg)))
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
        # What follows is a bit of a hack at the moment: rather than resolving
        # correctly combinations, we join the dictionaries. This will break
        # down as soon as combinations to a base class are added after those
        # of the derived classes.
        combinations = getattr(parent, 'combinations', None)
        combinations = ({} if combinations is None else combinations.copy())
        parent.combinations = combinations
    def combination_adder(fn):
        combinations[child] = fn
        if method_name is not None:
            setattr(parent, method_name, create_method(child, parent, fn))
        return fn
    return combination_adder
