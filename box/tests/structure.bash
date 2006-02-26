#!/bin/bash

. common.bash

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

echo "<< Testing how the compiler builds structures and species:"
rm -f $ERRORS

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
test_next
echo "testing structures with immediate values: (Int, Int)"
$BOX << EOF  >$BOXOUT 2>&1
  Print["answer=", (1, 2), '\n']
EOF
check_noerr
check_answer "(1, 2)"

#-------------------------
test_next
echo "testing structures with immediate values: (Real, Real)"
$BOX << EOF  >$BOXOUT 2>&1
  Print["answer=", (1.234, 9.876), '\n']
EOF
check_noerr
check_answer "(1.234, 9.876)"

#-------------------------
test_next
echo "testing structures with immediate values: (Int, Real)"
$BOX << EOF  >$BOXOUT 2>&1
  Print["answer=", (970, 9.876), '\n']
EOF
check_noerr
check_answer "(970, 9.876)"

#-------------------------
test_next
echo "testing structures with immediate values: (Real, Int)"
$BOX << EOF  >$BOXOUT 2>&1
  Print["answer=", (29.07, 1979), '\n']
EOF
check_noerr
check_answer "(29.07, 1979)"

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
test_next
echo "testing structures with variables as members"
$BOX << EOF  >$BOXOUT 2>&1
  dm = 29.07
  y = 1979
  Print["answer=", (dm, y), '\n']
EOF
check_noerr
check_answer "(29.07, 1979)"

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
test_next
echo "testing structures with registers as members"
$BOX << EOF  >$BOXOUT 2>&1
  d = 29
  m = 7
  y = 79
  Print["answer=", (d + m/100.0, y+1900), '\n']
EOF
check_noerr
check_answer "(29.07, 1979)"

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
test_next
echo "testing structures with mixed members"
$BOX << EOF  >$BOXOUT 2>&1
  y = 79
  Print["answer=", (29.07, y+1900), '\n']
EOF
check_noerr
check_answer "(29.07, 1979)"

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
echo ">>"
check_final
