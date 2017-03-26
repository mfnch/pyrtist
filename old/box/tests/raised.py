from test import TestSession

#----------------------------------------------------------------------------#
tests = TestSession(title="Raised")

#----------------------------------------------------------------------------#
test = tests.new_test(title="^X does not match combinations X@Y")
test.body = """X = ^Int, Print["answer=", X[0];]"""
test.expect(exit_status=0, num_errors=0, num_warnings=1, answer="")

#----------------------------------------------------------------------------#
test = tests.new_test(title="^x gives error, for x not a raised type")
test.body = """x = 456, Print["answer=", ^x;]"""
test.expect(exit_status=1, num_errors=1, num_warnings=0, answer=[])

#----------------------------------------------------------------------------#
test = tests.new_test(title="^x has type X, for x = (^X)[]")
test.body = """
X = ^Int
x = X[123]
Print["answer=", ^x;]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="123")
