#!/bin/bash

. common.bash

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

echo "<< Testing if some bugs are still present:"
rm -f $ERRORS

#------------------------------------------------------------------------------
test_next
echo "Bug 1: un-valued expressions are erroneously freed"
$BOX << EOF  >$BOXOUT 2>&1
Print[ pi = Atan[1]*4, pi; ]
EOF
check_noerr

#------------------------------------------------------------------------------
test_next
echo "Bug 2: The program \"Print[;]\\n\" gives a segmentation fault with option -o"
$BOX -o x << EOF  >$BOXOUT 2>&1
Print[ ; ]
EOF
check_noerr $?

#------------------------------------------------------------------------------
test_next
echo "Bug 3: The program with the new version of the VM sometimes gives a segfault"
$BOX -o x << EOF  >$BOXOUT 2>&1
a = 1.234
EOF
check_noerr $?

#------------------------------------------------------------------------------
test_next
echo "Bug 4: Internal error in Cmp_Structure_Get"
$BOX -o x << EOF  >$BOXOUT 2>&1
p = Point
p = (1, 2)
p = (1, 2)
p = (1, 2)
EOF
check_noerr $?

#------------------------------------------------------------------------------
test_next
echo "Bug 5: a = Point[.x=1, .y=-4]"
$BOX -o x << EOF  >$BOXOUT 2>&1
a = Point[.x=1, .y=-4]
EOF
check_noerr $?

echo ">>"
check_final
