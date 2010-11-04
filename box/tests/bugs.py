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

#----------------------------------------------------------------------------#
test = tests.new_test(title="bug 3")
test.body = """
MyObject = (Str one, two)
MyObject@MyObject[$$.one = $.two, $$.two = $.one]
Struc = (MyObject my_object, Int a)
(.[)@Struc[.my_object = MyObject[("two", "one")], .a = 1234]
struc = Struc[]
Print["answer=", struc.my_object.one, struc.my_object.two, struc.a;]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="onetwo1234")

#----------------------------------------------------------------------------#
test = tests.new_test(title="bug 4")
test.body = """
X = ++(Real ax, ay)    
X.GetPoint = Point
([)@X.GetPoint[$$.x = $$$.ax, $$.y = $$$.ay]
a = Point[.x=1.2, .y=2.3]
x = X[.ax=4.5, .ay=6.7]
a = x.GetPoint[]
Print["answer=", a;]
"""
test.expect(exit_status=0, num_errors=0, num_warnings=0, answer="(4.5, 6.7)")

#----------------------------------------------------------------------------#
test = tests.new_test(title="bug 5")
test.body = """
X = ++(Real x, y)
X.Y = Point
a = X.Y[] // Shouldn't be possible
"""
test.expect(exit_status=1, num_errors=1, num_warnings=0, answer=[])

