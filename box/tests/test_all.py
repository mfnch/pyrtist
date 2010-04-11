# -*- coding: utf-8 -*-

test_suites_modules = ['scope', 'ops', 'procdef', 'structure']

num_errors = 0
for test_suite_module in test_suites_modules:
  m = __import__(test_suite_module)
  num_errors += m.tests.run()

if num_errors > 0:
  import sys
  sys.exit(1)
