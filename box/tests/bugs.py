from test import TestSession                                                               
                                                                                           
#----------------------------------------------------------------------------#             
tests = TestSession(title="Bugs")

#----------------------------------------------------------------------------#
test = tests.new_test(title="testing solution of discovered bugs")
test.body = "a = x + y"
test.expect(exit_status=1, num_errors=2, num_warnings=0, answer=[])

