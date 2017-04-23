# Copyright (C) 2017 by Matteo Franchin (fnch@users.sf.net)
#
# This file is part of Pyrtist.
#
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


class Callbacks(object):
    '''Object which can be used to share callbacks among a family of
    objects.
    '''

    @classmethod
    def share(cls, prev_callbacks):
        if prev_callbacks is not None:
            if isinstance(prev_callbacks, Callbacks):
                return prev_callbacks
            return Callbacks(prev_callbacks)
        return cls()

    def __init__(self, callbacks_dict=None):
        if callbacks_dict is None:
            callbacks_dict = {}
        self._fns = callbacks_dict

    def __iter__(self):
        return iter(self._fns)

    def __setitem__(self, name, fn):
        self._fns[name] = fn

    def __getitem__(self, name):
        return self._fns[name]

    def define(self, name, fn=None):
        self._fns[name] = fn

    def redefine(self, name, fn=None):
        self._fns[name] = fn

    def default(self, name, fn=None):
        '''Provide the definition of a callback for the given name `name`.
        If a callback with the same name was already present, nothing is done.
        If `fn` is None, then a do-nothing callback is installed.
        '''
        self._fns.setdefault(name, fn)

    def provide(self, name, fn):
        '''Provide a definition for a callback which was already declared with
        the declare method.'''
        if name not in self._fns:
            raise ValueError("Cannot find '{}'. Available callbacks are: {}."
                             .format(name, ", ".join(self._fns.keys())))
        if self._fns[name] is not None:
            raise ValueError("Callback {} was already provided"
                             .format(name))
        self._fns[name] = fn

    def call(self, name, *args):
        fn = self._fns.get(name)
        if fn is None:
            return None
        return fn(*args)
