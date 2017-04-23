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

"""File which provides a function 'run_script' to run a Python script in the
background and allows to interact with it while it is running, e.g. get the
stdout, stderr or graphical output.
"""

import sys
import os
import imp
import io
import traceback
from threading import Thread
from multiprocessing import Process, Pipe


class PyrtistOutStream(io.IOBase):
  '''Stream used to pass output back to the Pyrtist GUI.'''

  def __init__(self, role, out_pipe):
    super(PyrtistOutStream, self).__init__()
    self._role = role
    self._out_pipe = out_pipe

  def writeable(self):
    return True

  def write(self, out_str):
    self._out_pipe.send((self._role, out_str))


class Redirected(object):
  '''Temporarily redirect stdout and stderr (use in a with statement).'''

  def __init__(self, stream_name, new_stream):
    self._stream_name = stream_name
    self._new_stream = new_stream
    self._old_stream = None

  def __enter__(self):
    self._old_stream = getattr(sys, self._stream_name)
    setattr(sys, self._stream_name, self._new_stream)
    return self._new_stream

  def __exit__(self, exc_type, exc_inst, exc_tb):
    setattr(sys, self._stream_name, self._old_stream)


def _run_code(send_to_parent, cwd, src_name, src, startup_cmds):
  # Run the script from the given working directory.
  if cwd is not None:
    if cwd == '':
      cwd = '.'
    try:
      os.chdir(cwd)
    except:
      pass
    else:
      sys.path.insert(0, os.getcwd())

  # Create the GUIGate object and allow it to populate the environment.
  from ..lib2d.gate import gui
  gui.connect(send_to_parent, startup_cmds)
  src_env = {}
  gui.update_vars(src_env)

  # Create a new namespace for the script execution.
  module = imp.new_module('__main__')
  code_globals = module.__dict__
  code_globals.update(__name__='__main__')
  code_globals.update(src_env)

  with Redirected('stdout', PyrtistOutStream('stdout', send_to_parent)):
    with Redirected('stderr', PyrtistOutStream('stderr', send_to_parent)):
      try:
        code = compile(src, src_name, 'exec')
        exec(code, code_globals)
      except Exception as exc:
        exc_type, exc_value, exc_tb = sys.exc_info()
        exc_tb = exc_tb.tb_next
        traceback.print_exception(exc_type, exc_value, exc_tb)

def _child_main(send_to_parent, *args):
  try:
    _run_code(send_to_parent, *args)
  finally:
    send_to_parent.send(('exit', None))

def _comm_with_child(process, callback, recv_from_child):
  timeout = 0.1
  while process.is_alive():
    items_in_queue = recv_from_child.poll(timeout)
    if not items_in_queue:
      continue

    try:
      cmd, value = recv_from_child.recv()
    except:
      cmd, value = ('eof', None)

    if cmd in ('stdout', 'stderr'):
      callback('out', cmd, value)
    elif cmd in ('exit', 'eof'):
      break
    else:
      callback(cmd, value)

  if process.is_alive():
    process.join()

  callback('exit')

def run_script(src_name, src, callback=None, cwd=None, startup_cmds=None):
  '''This function launches the Python script in `src` with name `src_name`
  in a separate task. The function creates a new thread to receive data
  from the running script. The data is passed back by using the provided
  `callback` function, like this `callback("out", stream_name, out_string)`,
  where `stream_name` is a string which identifies which stream the output
  is coming from  (it can either be "stdout" or "stderr") and `out_string`
  is a string containing the actual output. When the script terminates, a
  notification is sent back by calling `callback("exit")`.
  '''

  recv_from_child, send_to_parent = Pipe()
  p = Process(target=_child_main,
              args=(send_to_parent, cwd, src_name, src, startup_cmds))
  p.daemon = True
  p.start()

  t = Thread(target=_comm_with_child,
             args=(p, callback, recv_from_child))
  t.daemon = True
  t.start()

  return lambda: p.terminate()
