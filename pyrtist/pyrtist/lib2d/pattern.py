__all__ = ('Extend', 'Filter')

from .base import create_enum

doc = ('The pattern extend mode to be used for drawing outside '
       'the area of a pattern.')
Extend = create_enum('Extend', doc,
                     'unset', 'none', 'repeat', 'reflect', 'pad')

doc = ('Indicate what filtering should be applied when reading '
       'pixel values from patterns.')
Filter = create_enum('Filter', doc,
                     'unset', 'fast', 'good', 'best', 'nearest', 'bilinear',
                     'gaussian')
