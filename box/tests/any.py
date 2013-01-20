from test import TestSession

#----------------------------------------------------------------------------#
tests = TestSession(title="Any(boxing/unboxing)")

#----------------------------------------------------------------------------#
test = tests.new_test(title="Any of simple types (Int)")
test.body = """Print["answer=", Any[1234];]"""
test.expect(exit_status=0, num_errors=0, num_warnings=0,
            answer=1234)

#----------------------------------------------------------------------------#
test = tests.new_test(title="Print through Any")
test.body = """Print["answer=", Any["Hello world!"];]"""
test.expect(exit_status=0, num_errors=0, num_warnings=0,
            answer="Hello world!")

#----------------------------------------------------------------------------#
test = tests.new_test(title="Any as structure member")
test.body = """
X = (Any a, b)
x = X[.a="Hello", .b="world!"]
Print["answer=", x.a, x.b;]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0,
            answer="Helloworld!")
