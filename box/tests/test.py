# -*- coding: utf-8 -*-

import os
from subprocess import Popen, STDOUT, PIPE

def exec_cmd(cmd_args):
  p = Popen(cmd_args, stdout=PIPE, stderr=STDOUT)
  output = p.communicate()[0]
  return (output, p.returncode)

def test_print(level, msg):
  print msg

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
    test_print(0, 'SUITE: %s' % self.title)
    for test in self.tests:
      test_print(1, '  TEST: %s' % test.title)
      errors = test.run()
      if len(errors) > 0:
        for err in errors:
          test_print(2, err)

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
      if line.startswith("Error:"):
        num_errors += 1
      elif line.startswith("Warning:"):
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
      errors.append("===[Script source]===:\n%s\n===[Output was]===\n%s"
                    % (self.body, output))
    return errors
