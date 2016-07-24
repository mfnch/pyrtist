import types

def getClassName(obj):
    return obj.__class__.__name__


class RejectException(Exception):
    pass


class Taker(object):
    combinations = None

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
    if parent.combinations is None:
        parent.combinations = {}
    def combination_adder(fn):
        parent.combinations[child] = fn
        if method_name is not None:
            setattr(parent, method_name, create_method(child, parent, fn))
        return fn
    return combination_adder