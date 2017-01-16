"""File which provides a function 'exec_command' to run a program and continue
execution while it is running, using a different thread to get the output.
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


def _run_code(send_to_parent, cwd, src_name, src):
  if cwd is not None:
    os.chdir(cwd)
    sys.path.insert(0, os.getcwd())

  src_env = {}

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

def _comm_with_child(process, out_fn, do_at_exit, recv_from_child):
  timeout = 0.1
  while process.is_alive():
    items_in_queue = recv_from_child.poll(timeout)
    if not items_in_queue:
      continue

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

def exec_command(src_name, src, out_fn=None, do_at_exit=None, cwd=None):
  '''This function launches the Python script in `src` with name `src_name`
  in a separate task. The function creates a new thread to read the output
  of the program and returns while this may still be running. The output is
  red in chunks of size not greater than `buffer_size` and is passed back
  using the callback function `out_fn` (like this: `out_fn(nth_chunk)`).
  Once the running program terminates `do_at_exit()` is called (if it is
  provided).
  '''

  recv_from_child, send_to_parent = Pipe()
  p = Process(target=_child_main, args=(send_to_parent, cwd, src_name, src))
  p.daemon = True
  p.start()

  t = Thread(target=_comm_with_child,
             args=(p, out_fn, do_at_exit, recv_from_child))
  t.daemon = True
  t.start()

  return lambda: p.terminate()
