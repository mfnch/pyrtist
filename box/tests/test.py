# -*- coding: utf-8 -*-

import os
from subprocess import Popen, STDOUT, PIPE

def exec_cmd(cmd_args):
  p = Popen(cmd_args, stdout=PIPE, stderr=STDOUT)
  output = p.communicate()[0]
  return (output, p.returncode)

class XTermColors:
  def __init__(self):
    self.red   = '\033[00;31m'
    self.green = '\033[00;32m'
    self.yellow = '\033[00;33m'
    self.no    = '\033[0m'

colors = XTermColors()

color_of_level = {
  'normal' : None,
  'success': colors.green,
  'error'  : colors.red,
  'info'   : colors.yellow
}

def test_print(level, msg, endline=''):
  activate_color = ""
  deactivate_color = ""

  if color_of_level.has_key(level):
    c = color_of_level[level]
    if c != None:
      activate_color = c
      deactivate_color = colors.no

  print activate_color + msg + deactivate_color + endline,

def test_printl(level, msg, endline='\n'):
  test_print(level, msg, endline)

def check(error, thing_name, expectations, obtained_thing):
  if expectations.has_key(thing_name):
    expected_thing = expectations[thing_name]
    if expected_thing != None:
      if isinstance(expected_thing, str):
        assert expected_thing.lower().startswith("not")
        ok = (int(expected_thing[3:]) != obtained_thing)
      else:
        ok = (obtained_thing == expected_thing)

      if not ok:
        error.append("Expected %s=%s, but got %s"
                     % (thing_name, expected_thing, obtained_thing))

class TestSession:
  def __init__(self, title=None, box_exec="../box/box",
               test_source="test_source.box"):
    self.title = title
    self.box_exec = box_exec
    self.test_source = test_source
    self.tests = []

  def new_test(self, title=None):
    test = Test(title=title,
                box_exec=self.box_exec,
                test_source=self.test_source)
    self.tests.append(test)
    return test

  def run(self):
    test_printl('normal', 'SUITE: %s' % self.title)
    num_errors = 0
    for test in self.tests:
      test_print('normal', '  TEST: %50s' % test.title)
      errors = test.run()
      num_errors += len(errors)
      if len(errors) == 0:
        test_printl('success', '[SUCCESS]')

      else:
        test_printl('error', '[FAILED]')
        for err in errors:
          test_printl('info', err)
    return num_errors

class Test:
  def __init__(self, title=None, box_exec="box",
               test_source="test_source.box"):
    self.title = title
    self.box_exec = box_exec
    self.test_source = test_source

    self._expect = {}
    self.body = None
    self.opts = []

  def add_opts(self, opt_list):
    self.opts += opt_list

  def expect(self, exit_status=None, num_errors=None, num_warnings=None,
             answer=None):
    self._expect['exit_status'] = exit_status
    self._expect['num_errors'] = num_errors
    self._expect['num_warnings'] = num_warnings
    self._expect['answer'] = answer

  def write_body(self):
    if self.body == None:
      raise ValueError("Cannot write test file: test has no body.")
    f = open(self.test_source, "w")
    f.write(self.body)
    f.close()
  
  def run(self):
    self.write_body()

    output, exit_status = \
      exec_cmd([self.box_exec, self.test_source] + self.opts)

    errors = []

    check(errors, 'exit_status', self._expect, exit_status)

    num_errors = 0
    num_warnings = 0
    answer = []
    lines = output.splitlines()
    for line in lines:
      if line.startswith("Error"):
        num_errors += 1
      elif line.startswith("Warning"):
        num_warnings += 1
      elif line.startswith("answer"):
        answer.append(line.split('=', 1)[1])

    check(errors, 'num_warnings', self._expect, num_warnings)
    check(errors, 'num_errors', self._expect, num_errors)

    if self._expect.has_key('answer'):
      if len(answer) == 1:
        answer = answer[0]
      if self._expect['answer'] != answer:
        errors.append('Expected answer="%s", but got "%s"'
                      % (self._expect['answer'], answer))

    if len(errors) > 0:
      errors.append("===[Script source]===:\n%s\n===[Output was]===\n%s\n===[End]==="
                    % (self.body, output))
    return errors
