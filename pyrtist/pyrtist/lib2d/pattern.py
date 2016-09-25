__all__ = ('Extend', 'Filter')

from .base import create_enum, Taker, combination
from .cmd_stream import Cmd, CmdStream

doc = '''Extend mode to be used for drawing outside the pattern area.

Available options are:
- Extend.none: pixels outside of the source pattern are fully transparent,
- Extend.repeat: the pattern is tiled by repeating,
- Extend.reflect: the pattern is tiled by reflecting at the edges,
- Extend.pad: pixels outside of the pattern copy the closest pixel from the
  source.
'''
Extend = create_enum('Extend', doc,
                     'unset', 'none', 'repeat', 'reflect', 'pad')

doc = '''Filtering to  be applied when reading pixel values from patterns.

Available values are:
- Filter.fast: a high-performance filter, with quality similar to
  Filter.nearest,
- Filter.good: a reasonable-performance filter, with quality similar to
  Filter.bilinear,
- Filter.best: the highest-quality  available, performance may not be
  suitable for interactive use,
- Filter.nearest: nearest-neighbor filtering,
- Filter.bilinear: linear interpolation in two dimensions
- Filter.gaussian: this filter value is currently unimplemented, and should
  not be used in current code.
'''
Filter = create_enum('Filter', doc,
                     'unset', 'fast', 'good', 'best', 'nearest', 'bilinear',
                     'gaussian')

class Pattern(object):
    '''The "paint" to be used when filling a shape or drawing a line.
    `Color` and `Gradient` are automatically converted to `Pattern`.
    This means that whenever a Pattern can be provided, a `Color` (or
    `Gradient`) can also be provided.
    '''

    def _get_cmds(self):
        # This method should be overloaded by the inheriting classes and should
        # return a list of commands necessary to set up the specific pattern.
        return []

@combination(Pattern, CmdStream)
def pattern_at_cmd_stream(pattern, cmd_stream):
    cmd_stream.take(*pattern._get_cmds())
