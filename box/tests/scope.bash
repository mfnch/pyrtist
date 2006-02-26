#!/bin/bash
BOX="../box"
BOXOPT="-st"
BOXOUT="output.log"
ERRORS="errors.log"
NUM_TEST=0
NUM_ERRORS=0
ERR_REPORTED=0

function err_report {
  if [ $ERR_REPORTED -eq 0 ]; then
    echo "*** Beginning of test $NUM_TEST (ERRORS DETECTED!) ***" >> $ERRORS
    cat $BOXOUT >> $ERRORS
    echo "*** End of test $NUM_TEST ***" >> $ERRORS
    echo >> $ERRORS
    ERR_REPORTED=1
  fi
}

function test_next {
  NUM_TEST=$[ $NUM_TEST + 1 ]
  echo
  echo -n "  < Test $NUM_TEST: "
  ERR_REPORTED=0
}

function check_successful {
  echo "  > Test successful!"
}

function check_failed {
  echo "  > --> Test FAILED! <--"
}

function check_noerr {
  local ne=$(grep Err $BOXOUT | wc -l)
  NUM_ERRORS=$[ $NUM_ERRORS + $ne ]
  if [ $ne -lt 1 ]; then
    check_successful
  else
    err_report
    check_failed
  fi
}

function check_answer {
  local ANSWER=$(grep "answer=" $BOXOUT | cut -d = -f 2)
  if [ "$ANSWER" == "$1" ]; then
    check_successful
  else
    err_report
    check_failed
  fi
}

function check_final {
  echo "$NUM_ERRORS errors found."
  if [ $NUM_ERRORS -lt 1 ]; then
    echo "All tests performed led to success."
    exit 0
  fi
  echo "See the file '$ERRORS' for more details!"
  exit 1
}

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
if [ -x $BOX ]; then
  BOX="$BOX $BOXOPT"
else
  echo "I cannot find the executable file '$BOX'!"
  exit 1
fi

echo "<< Testing if explicit variables are killed when their box is closed:"
rm -f $ERRORS

#------------------------------------------------------------------------------
test_next
echo "testing untyped boxes, explicit variables without value"
$BOX << EOF  >$BOXOUT 2>&1
  [a = CHAR]
  a = INT
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
  a = CHAR
EOF
check_noerr

#------------------------------------------------------------------------------
test_next
echo "testing typed boxes, explicit variables"
$BOX << EOF  >$BOXOUT 2>&1
  Print[a = 1.234]
  a = CHAR
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
