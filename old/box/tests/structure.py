# -*- coding: utf-8 -*-

from test import TestSession

#----------------------------------------------------------------------------#
tests = TestSession(title="Structure")

#----------------------------------------------------------------------------#
test = tests.new_test(title="structures with immediate values: (Int, Int)")
test.body = """Print["answer=", (1, 2);]"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(1, 2)")

#----------------------------------------------------------------------------#
test = tests.new_test(title="structures with immediate values: (Real, Real)")
test.body = """Print["answer=", (1.234, 9.876);]"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(1.234, 9.876)")

#----------------------------------------------------------------------------#
test = tests.new_test(title="structures with immediate values: (Int, Real)")
test.body = """Print["answer=", (970, 9.876);]"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(970, 9.876)")

#----------------------------------------------------------------------------#
test = tests.new_test(title="structures with immediate values: (Real, Int)")
test.body = """Print["answer=", (44.33, 1987);]"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(44.33, 1987)")

#----------------------------------------------------------------------------#
test = tests.new_test(title="structures with variables as members")
test.body = """
  dm = 44.33
  y = 1987
  Print["answer=", (dm, y);]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(44.33, 1987)")

#----------------------------------------------------------------------------#
test = tests.new_test(title="structures with registers as members")
test.body = """
  d = 44
  m = 33
  y = 87
  Print["answer=", (d + m/100.0, y+1900);]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(44.33, 1987)")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing structures with mixed members")
test.body = """
  y = 87
  Print["answer=", (44.33, y+1900);]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(44.33, 1987)")

#----------------------------------------------------------------------------#
test = tests.new_test(title="'Point': conversion structure --> Point")
test.body = """
  p1 = (1, 2.0)
  p2 = -(3.5, 4.0)
  p3 = p1 + p2
  Print["answer=", p3;]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(-2.5, -2)")

#----------------------------------------------------------------------------#
test = tests.new_test(title="'Point': operations")
test.body = """
  p1 = (1.0, -2) - (3, -4.0) + (5, -6) - (7.0, -8.0)
  p2 = p1/2 + p1/4.0 + p1*4 - p1*5.0 + 6.0*p1 - 7*p1
  Print["answer=", (p2.y, p2.x / 1.25);]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(-5, 4)")

#----------------------------------------------------------------------------#
test = tests.new_test(title="Implicit members of structs 1")
test.body = """
  X = (Int x, Real y, Point z)
  r = X[.x = 123, .y=4.56, .z=(7, 8.9)]
  Print["answer=", r.x; "answer=", r.y; "answer=", r.z;]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0,
            answer=["123", "4.56", "(7, 8.9)"])

#----------------------------------------------------------------------------#
test = tests.new_test(title="Implicit members of structs 2")
test.body = """
  X = (Int x, (Real u, Point w) uw)
  r = X[.x = 12, .uw.u=4.5, .uw.w=(7.2, 8.9)]
  Print["answer=", r.x; "answer=", r.uw.u; "answer=", r.uw.w;]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0,
            answer=["12", "4.5", "(7.2, 8.9)"])

#----------------------------------------------------------------------------#
test = tests.new_test(title="Implicit members of point")
test.body = """
  p = Point[.x = 12, .y=3.456]
  Print["answer=", p; "answer=", Point[.x = 7.8, .y=9];]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0,
            answer=["(12, 3.456)", "(7.8, 9)"])