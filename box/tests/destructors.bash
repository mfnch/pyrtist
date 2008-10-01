#!/bin/bash

. common.bash

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

echo "<< Testing if some destructors are properly called:"
rm -f $ERRORS

#------------------------------------------------------------------------------
test_next
echo "Test 1: destructor for an independently allocated object (non member)"
$BOX << EOF  >$BOXOUT 2>&1
MyObj = (Int a, b)
(\)@MyObj[Print["answer=destroyed";]]
my_obj = MyObj[]
EOF
check_noerr
check_answer "destroyed"

#------------------------------------------------------------------------------
test_next
echo "Test 2: another test"
$BOX -o x << 'EOF'  >$BOXOUT 2>&1
num_of_MyObjs = 0
MyObj = (Int n,)
([)@MyObj[$$.n = ++num_of_MyObjs]
(\)@MyObj[Print["obj", $$.n]]
Print["answer="]
my_obj1 = MyObj[]
my_obj2 = MyObj[]
my_obj3 = MyObj[]
EOF
check_noerr $?
check_answer "obj3obj2obj1"

echo ">>"
check_final
