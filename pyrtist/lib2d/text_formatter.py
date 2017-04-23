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

from cStringIO import StringIO

__all__ = ('TextFormatter',)


class TextFormatter(object):
    def __init__(self):
        self.max_stack_level = 10
        self.out = StringIO()
        self.states = []
        self.level = 0
        self.string = None
        self.cursor = 0

    def push_state(self, old_state, new_state):
        self.states.append(old_state)
        return new_state

    def pop_state(self):
        return self.states.pop()

    def pop_text(self):
        ret = self.out.getvalue()
        self.out.reset()
        self.out.truncate()
        return ret

    def run(self, string):
        self.cursor = 0
        self.string = string
        fn = self._state_normal
        while self.cursor < len(self.string):
            c = self.string[self.cursor]
            fn = fn(c)
        self.cmd_draw()

    def _state_normal(self, c):
        self.cursor += 1
        if c == '_':
            return self._state_wait_sub
        if c == '^':
            return self._state_wait_sup
        if c == '\\':
            return self.push_state(self._state_normal, self._state_literal)

        if c == '}':
            if self.level > 0:
                self.cmd_draw()
                self.cmd_restore()
                self.level -= 1
                return self._state_normal
        elif c == '\n':
            self.cmd_draw()
            self.cmd_newline()
        else:
            self.out.write(c)
        return self._state_normal

    def _state_single(self, c):
        self.cursor += 1
        if c == '\n':
            # Ignore newlines.
            return self._state_single
        if c == '\\':
            return self.push_state(self._state_single, self._state_literal)
        self.out.write(c)
        self.cmd_draw()
        self.cmd_restore()
        return self._state_normal

    def _state_literal(self, c):
        self.cursor += 1
        self.out.write(c)
        return self.pop_state()

    def _state_wait_sup(self, c):
        return self._state_wait_sub(c, sup=True)

    def _state_wait_sub(self, c, sup=False):
        self.cursor += 1
        if c in '_^':
            if (c == '^') == sup:
                self.out.write(c)
                return self._state_normal

        self.cmd_draw()
        self.cmd_save()
        if sup:
            self.cmd_superscript()
        else:
            self.cmd_subscript()

        if c != '{':
            self.cursor -= 1
            return self._state_single
        self.level += 1
        return self._state_normal
