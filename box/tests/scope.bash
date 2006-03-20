#!/bin/bash

. common.bash

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

echo "<< Testing if explicit variables are killed when their box is closed:"
rm -f $ERRORS

#------------------------------------------------------------------------------
test_next
echo "testing untyped boxes, explicit variables without value"
$BOX << EOF  >$BOXOUT 2>&1
  [a = Char]
  a = Int
EOF
check_noerr

#------------------------------------------------------------------------------
test_next
echo "testing again with complex expressions"
$BOX << EOF  >$BOXOUT 2>&1
  pi = 3.1415926
  x = pi/2.0
  [x2 = x*x, x3 = x2*x, x5 =x3*x2, x7=x5*x2
  f3=6.0, f5=f3*4.0*5.0, f7=f5*6.0*7.0
  y::=x-x3/f3+x5/f5-x7/f7]
  Print["answer=", y]
  x2=1, x3=1, x5=6, x7=9
  f3='a', f5='b', f7='r'
EOF
check_noerr
check_answer 0.999843

#------------------------------------------------------------------------------
test_next
echo "testing untyped boxes, explicit variables with value"
$BOX << EOF  >$BOXOUT 2>&1
  [a = 1.234]
  a = Char
EOF
check_noerr

#------------------------------------------------------------------------------
test_next
echo "testing typed boxes, explicit variables"
$BOX << EOF  >$BOXOUT 2>&1
  Print[a = 1.234]
  a = Char
EOF
check_noerr

#------------------------------------------------------------------------------
test_next
echo "testing again with complex expressions"
$BOX << EOF  >$BOXOUT 2>&1
  pi = 3.1415926
  x = pi/2.0
  Print[x2 = x*x, x3 = x2*x, x5 =x3*x2, x7=x5*x2
  f3=6.0, f5=f3*4.0*5.0, f7=f5*6.0*7.0
  "answer=", x-x3/f3+x5/f5-x7/f7]
  x2=1, x3=1, x5=6, x7=9
  f3='a', f5='b', f7='r'
EOF
check_noerr
check_answer 0.999843

#------------------------------------------------------------------------------
test_next
echo "testing scoping and unnamed-prefixes of explicit symbols"
$BOX << EOF  >$BOXOUT 2>&1
Print[
  a = 1.23
  [
    Print[
      a: = 4.56
      Print[
        b = a::
        c = a::::
        Print["answer=", b, ',', c, '\n']
      ]
    ]
  ]
]
EOF
check_noerr
check_answer 4.56,1.23

#------------------------------------------------------------------------------
test_next
echo "testing scoping and named-prefixes of explicit symbols"
$BOX << EOF  >$BOXOUT 2>&1
Print[
  a = 1.23
  [
    Print[
      a: = 4.56
      Print[
        b = a:Print
        c = a:Print:Print
        Print["answer=", b, ',', c, '\n']
      ]
    ]
  ]
]
EOF
check_noerr
check_answer 4.56,1.23

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
echo ">>"
check_final
