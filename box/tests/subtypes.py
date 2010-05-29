from test import TestSession

#----------------------------------------------------------------------------#
tests = TestSession(title="Subtypes")

#----------------------------------------------------------------------------#
test = tests.new_test(title="Testing Obj.Void")
test.body = """
  R = (Real x, y, z)
  R.Print = Void
  ([)@R.Print[Print['(', $$$.x, (v=", "), $$$.y, v, $$$.z, ')']]

  r = R[] //.x=1, .y=2, .z=3]
  r.x = 1, r.y = 2, r.z = 3 // temp fix

  Print["answer=", \ r.Print[];]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(1, 2, 3)")

#----------------------------------------------------------------------------#
test = tests.new_test(title="Testing Obj.Obj")
test.body = """
  R = (Real x, y, z)
  R.Get_zy = Point
  ([)@R.Get_zy[
    Print[$$$.z]
    //$$.x = $$$.y, $$.y = $$$.z
  ]

  r = R[] //.x=1, .y=2, .z=3]
  r.x = 1, r.y = 2, r.z = 3 // temp fix

  Print["answer=", r.Get_zy[];]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(2, 3)")
