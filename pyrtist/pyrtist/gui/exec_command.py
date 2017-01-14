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

def exec_command(args=[], out_fn=None, do_at_exit=None,
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
  po = Popen([cmd] + args, stdin=PIPE, stdout=PIPE, stderr=STDOUT,
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
