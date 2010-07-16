from test import TestSession                                                               
                                                                                           
#----------------------------------------------------------------------------#             
tests = TestSession(title="Bugs")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing solution of discovered bugs: bug 1")
test.body = "a = x + y, b = +z"
test.expect(exit_status=1, num_errors=3, num_warnings=0, answer=[])

#----------------------------------------------------------------------------#
test = tests.new_test(title="bug 2")
test.body = """
v = (1, 2)
X = Void
([)@X[Print["answer=", v;]]
X[]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(1, 2)")
