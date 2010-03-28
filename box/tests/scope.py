# -*- coding: utf-8 -*-

from test import TestSession

#----------------------------------------------------------------------------#
tests = TestSession(title="Scope")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing scope of procedure definitions: 1")
test.body = """
X = Void
[Int@X[Print["answer=X"]], X[1]]
[Int@X[Print["Y";]], X[1]]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="XY")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing scope of procedure definitions: 2")
test.add_opts(['-f'])
test.body = """
X = Void
[Int@X[Print["answer=X"]], X[1]]
X[1]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=1, answer="X")

#----------------------------------------------------------------------------#

tests.run()
