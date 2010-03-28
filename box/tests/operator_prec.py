# -*- coding: utf-8 -*-

from test import TestSession

#----------------------------------------------------------------------------#
tests = TestSession(title="Operator precedence")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing for precedence of + and *")
test.body = """
Print["answer=", (2 + 4*5) == (2 + (4*5));]
Print["answer=", (2 + 4*5) != ((2 + 4)*5);]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer=['1', '1'])

#----------------------------------------------------------------------------#

tests.run()
