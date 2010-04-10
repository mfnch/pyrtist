# -*- coding: utf-8 -*-

from test import TestSession

#----------------------------------------------------------------------------#
tests = TestSession(title="ProcDef")

dylib_flags = ['-L', './dylib', '-l', 'mylib']

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing scope of procedure definitions: 1")
test.body = """
X = Void
Print["answer="]
[Int@X[Print["X"]], X[1]]
[Int@X[Print["Y"]], X[1]]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="XY")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing scope of procedure definitions: 2")
test.add_opts(['-f'])
test.body = """
X = Void
Print["answer="]
[Int@X[Print["X"]], X[1]]
X[1]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=1, answer="X")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing overlapping procedure definitions")
test.body = """
X = Void
Print["answer="]
Int@X[Print["X"]], X[1]
Int@X[Print["Y"]], X[1]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="XY")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing procedure declarations")
test.body = """
X = Void
Print["answer="]
Int@X?, X[1]
Int@X[Print["X"]]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="X")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing external C procedures: simplest")
test.add_opts(dylib_flags)
test.body = """
Print["answer="]
X = Void
Int@X "mylib_simple" ?
X[1]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="simple")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing external C procedures: simple 2")
test.add_opts(dylib_flags)
test.body = """
X = Void, Y = Void
Str@X "mylib_print_str_a" ?
Str@Y "mylib_print_str_b" ?
Print["answer="]
X["1"], Y["2"]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="A:1B:2")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing external C procedures: post definition")
test.add_opts(dylib_flags)
test.body = """
Print["answer="]
[X = Void, Str@X ?
 X["1"]
 Str@X "mylib_print_str_a" ?]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="A:1")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing external C procedures: redefinition")
test.add_opts(dylib_flags)
test.body = """
Print["answer="]
X = Void, Str@X ?
X["1"]
Str@X "mylib_print_str_a" ?
X["2"]
Str@X ?
X["3"]
Str@X "mylib_print_str_b" ?
X["4"]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0,
            answer="A:1A:2A:3B:4")
