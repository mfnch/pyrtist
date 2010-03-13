import os
import config

CREATE_NO_WINDOW = 0x08000000

def old_exec_command(cmd, args=[], out_fn=None, do_at_exit=None):
  # This is not really how we should deal properly with this.
  # (We should avoid .read()). It is just a temporary solution.

  from subprocess import Popen, PIPE, STDOUT

  # Fix for the issue described at 
  # http://www.py2exe.org/index.cgi/Py2ExeSubprocessInteractions
  if config.platform_is_win:
    shell = True
    creationflags = 0x08000000 # CREATE_NO_WINDOW

  else:
    shell = False
    creationflags = 0

  po = Popen([cmd] + args, stdin=PIPE, stdout=PIPE, stderr=STDOUT,
             shell=shell, creationflags=creationflags)

  content = po.stdout.read()
  po.stdout.close()
  po.stdin.close()

  if out_fn != None:
    out_fn(content)
  if do_at_exit != None:
    do_at_exit()

  return None

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

try:
  from threading import Thread
  from subprocess import Popen, PIPE, STDOUT

  def exec_command(cmd, args=[], out_fn=None, do_at_exit=None,
                   buffer_size=1024):

    # Fix for the issue described at 
    # http://www.py2exe.org/index.cgi/Py2ExeSubprocessInteractions
    if config.platform_is_win_py2exe:
      shell = True
      creationflags = CREATE_NO_WINDOW

    elif config.platform_is_win:
      shell = False
      creationflags = CREATE_NO_WINDOW
      
    else:
      shell = False
      creationflags = 0

    po = Popen([cmd] + args, stdin=PIPE, stdout=PIPE, stderr=STDOUT,
               shell=shell, creationflags=creationflags)

    # The following function will run on a separate thread
    def read_box_output(out_fn, do_at_exit):
      while True:
        out_str = po.stdout.readline(buffer_size)
        if len(out_str) == 0: # po.poll() == None and
          break

        if out_fn != None:
          out_fn(out_str)

      if do_at_exit != None:
        do_at_exit()

    args_to_target = (out_fn, do_at_exit)

    t = Thread(target=read_box_output, args=args_to_target)
    t.daemon = True
    t.start()

    return _killer(po)

except:
  exec_command = old_exec_command

if not config.use_threads:
  exec_command = old_exec_command

