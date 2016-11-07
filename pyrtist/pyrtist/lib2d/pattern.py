'''Definition of Pattern, the paint to use for draw and fill operations.'''

__all__ = ('Pattern',)

from .base import combination
from .cmd_stream import Cmd, CmdStream


class Pattern(object):
    '''The source to use when filling a shape or drawing a line.  `Color` and
    `Gradient` are derived classes.  This means that whenever a Pattern can be
    provided, a `Color` (or `Gradient`) can also be provided.
    '''

    def _get_cmds(self):
        # This method should be overloaded by the inheriting classes and should
        # return a list of commands necessary to set up the specific pattern.
        return []


@combination(Pattern, CmdStream)
def pattern_at_cmd_stream(pattern, cmd_stream):
    cmd_stream.take(*pattern._get_cmds())
