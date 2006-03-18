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
  if [ "$1" == "" ]; then
    echo "  > Test successful!"
  else
    echo "  > Test successful: $1"
  fi
}

function check_failed {
  echo "  > --> Test FAILED! <--"
}

function check_noerr {
  local ne=$(grep Err $BOXOUT | wc -l)
  NUM_ERRORS=$[ $NUM_ERRORS + $ne ]
  if [ $ne -lt 1 ]; then
    check_successful "No errors."
  else
    err_report
    check_failed
  fi
}

function check_answer {
  local ANSWER=$(grep "answer=" $BOXOUT | cut -d = -f 2)
  if [ "$ANSWER" == "$1" ]; then
    check_successful "Right result."
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
if [ -x $BOX ]; then
  BOX="$BOX $BOXOPT"
else
  echo "I cannot find the executable file '$BOX'!"
  exit 1
fi
