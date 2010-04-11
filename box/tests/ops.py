# -*- coding: utf-8 -*-

from test import TestSession

#----------------------------------------------------------------------------#
tests = TestSession(title="Operators")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing for precedence of + and *")
test.body = """
Print["answer=", (2 + 4*5) == (2 + (4*5));]
Print["answer=", (2 + 4*5) != ((2 + 4)*5);]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer=['1', '1'])

#----------------------------------------------------------------------------#
test = tests.new_test(title="assignment is right associative")
test.body = """
a = 1, b = 2, c = 3
a = b = c
Print["answer=", a, b, c]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer='333')

#----------------------------------------------------------------------------#
test = tests.new_test(title="assignment returns rhs")
test.body = """
a = b = 12345, Print["answer=", a == b;]
c = d = 1.234, Print["answer=", c == d;]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer=['1', '1'])

