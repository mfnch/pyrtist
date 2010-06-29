# -*- coding: utf-8 -*-

from test import TestSession

#----------------------------------------------------------------------------#
tests = TestSession(title="Scope")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing scope of variables: 1")
test.body = "[a = Char[]], a = Int[]"
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer=[])

#----------------------------------------------------------------------------#
#test = tests.new_test(title="testing scope of variables: 2")
#test.body = 
"""
  pi = 3.1415926
  x = pi/2.0
  [x2 = x*x, x3 = x2*x, x5 =x3*x2, x7=x5*x2
   f3=6.0, f5=f3*4.0*5.0, f7=f5*6.0*7.0
   y::=x-x3/f3+x5/f5-x7/f7]
  Print["answer=", y]
  x2=1, x3=1, x5=6, x7=9
  f3='a', f5='b', f7='r'
"""
#test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="0.999843")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing scope of variables: 2")
test.body = """
  pi = 3.1415926
  x = pi/2.0
  Print[x2 = x*x, x3 = x2*x, x5 =x3*x2, x7=x5*x2
        f3=6.0, f5=f3*4.0*5.0, f7=f5*6.0*7.0
        "answer=", x-x3/f3+x5/f5-x7/f7]
  x2=1, x3=1, x5=6, x7=9
  f3="a", f5='b', f7='r'
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="0.999843")

