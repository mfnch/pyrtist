# Copyright (C) 2011 by Matteo Franchin (fnch@users.sourceforge.net)
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


class SimpleStream(object):
  '''Simple stream which stores the messages it receives.'''

  def __init__(self):
    self.msgs = []
  
  def write(self, msg):
    self.msgs.append(msg)

  def get_msgs(self):
    return self.msgs


class SimpleLogger(object):
  def __init__(self, stream=None):
    self.context = None
    self.stream = stream

  def set_context(self, context):
    self.context = context

  def write(self, msg):
    if not self.stream:
      return

    if self.context:
      self.stream.write("%s: %s\n" % (self.context, msg))
    else:
      self.stream.write("%s\n" % msg)


import sys
logger = SimpleLogger(stream=None)

def set_log_context(context):
  '''Set the context (e.g. file being processed) to which the log messages
  (provided via 'log_msg') refer.'''
  if logger:
    logger.set_context(context)
  
def log_msg(msg):
  '''Log a message.'''
  if logger:
    logger.write(msg)
