"""File which provides a function 'exec_command' to run a program and continue
execution while it is running, using a different thread to get the output.
"""

import os
from threading import Thread
from subprocess import Popen, PIPE, STDOUT

import config


CREATE_NO_WINDOW = 0x08000000

if config.platform_is_win:
  def _my_killer(po):
    try:
      from win32process import TerminateProcess
      TerminateProcess(po._handle, 1)
    except:
      pass
else:
  def _my_killer(po):
    try:
      from os import kill
      import signal
      kill(po.pid, signal.SIGTERM)
    except:
      pass

def _killer(po):
  if hasattr(po, 'kill'):
    return po.kill
  else:
    return lambda: _my_killer(po)

def old_exec_command(src_filename, out_fn=None, do_at_exit=None,
                     buffer_size=1024, cwd=None):
  """This function launches the command 'cmd' (an executable file) with the
  given command line arguments 'args'. The function creates a new thread
  to read the output of the program and returns while this may still be
  running. The output is red in chunks of size not greater than
  'buffer_size' and is passed back using the callback function 'out_fn'
  (like this: 'out_fn(nth_chunk)'). Once the running program terminates
  'do_at_exit()' is called (if it is provided)."""

  if config.platform_is_win:
    creationflags = CREATE_NO_WINDOW
  else:
    creationflags = 0

  cmd = '/usr/bin/python'
  po = Popen([cmd, src_filename], stdin=PIPE, stdout=PIPE, stderr=STDOUT,
             shell=False, creationflags=creationflags, cwd=cwd)

  # The following function will run on a separate thread
  def read_box_output(out_fn, do_at_exit):
    while True:
      out_str = po.stdout.readline(buffer_size)
      if len(out_str) == 0: # po.poll() == None and
        break

      if out_fn is not None:
        out_fn(out_str)

    if do_at_exit is not None:
      do_at_exit()

  t = Thread(target=read_box_output, args=(out_fn, do_at_exit))
  t.daemon = True
  t.start()

  return _killer(po)


import sys
import io
import traceback
from threading import Thread
from multiprocessing import Process, Pipe


class PyrtistOutStream(io.IOBase):
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


def _child_main(cwd, file_name, send_to_parent):
  os.chdir(cwd)

  with open(file_name) as f:
    script_body = f.read()
  code = compile(script_body, file_name, 'exec')

  error_num = 1
  with Redirected('stdout', PyrtistOutStream('stdout', send_to_parent)):
    with Redirected('stderr', PyrtistOutStream('stderr', send_to_parent)):
      global_vars = globals().copy()
      try:
        exec(code, global_vars, global_vars)
        error_num = 0
      except Exception as exc:
        exc_type, exc_value, exc_tb = sys.exc_info()
        exc_tb = exc_tb.tb_next
        traceback.print_exception(exc_type, exc_value, exc_tb)
      finally:
        send_to_parent.send(('exit', error_num))

def _comm_with_child(process, out_fn, do_at_exit, recv_from_child):
  while process.is_alive():
    try:
      cmd, value = recv_from_child.recv()
    except EOFError:
      cmd, value = ('eof', None)

    if cmd in ('stdout', 'stderr'):
      out_fn(value)
    elif cmd == 'exit':
      process.join()
      break

  if do_at_exit is not None:
    do_at_exit()

def exec_command(src_filename, out_fn=None, do_at_exit=None,
                 buffer_size=1024, cwd=None):
  """This function launches the command 'cmd' (an executable file) with the
  given command line arguments 'args'. The function creates a new thread
  to read the output of the program and returns while this may still be
  running. The output is red in chunks of size not greater than
  'buffer_size' and is passed back using the callback function 'out_fn'
  (like this: 'out_fn(nth_chunk)'). Once the running program terminates
  'do_at_exit()' is called (if it is provided)."""

  recv_from_child, send_to_parent = Pipe()
  p = Process(target=_child_main, args=(cwd, src_filename, send_to_parent))
  p.daemon = True
  p.start()

  t = Thread(target=_comm_with_child,
             args=(p, out_fn, do_at_exit, recv_from_child))
  t.daemon = True
  t.start()

  return lambda: p.terminate()
