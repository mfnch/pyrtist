#!/bin/bash
BOXEXEC="../src/box"
BOXOPTLG="-L ../libs/g/.libs -I ../libs/g -l g"
BOXOPTST="-st"
BOXOUT="output.log"
ERRORS="errors.log"
NUM_TEST=0
NUM_ERRORS=0
ERR_REPORTED=0

# Setup colors
case $TERM in
  xterm*|rxvt*)
    RED=$(echo -e '\033[00;31m')
    GREEN=$(echo -e '\033[00;32m')
    NO_COLOUR=$(echo -e '\033[0m')
    ;;
  *)
    GREEN=""
    RED=""
    NO_COLOUR=""
    ;;
esac

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
    echo "  > $GREEN""Test successful!"$NO_COLOUR
  else
    echo "  > $GREEN""Test successful"$NO_COLOUR": $1"
  fi
}

function check_failed {
  if [ "$1" == "" ]; then
    echo "  > "$RED"--> TEST FAILED! <--"$NO_COLOUR
  else
    echo "  > "$RED"--> TEST FAILED! <--"$NO_COLOUR": $1"
  fi
}

function check_noerr {
  local ne=$(grep Err $BOXOUT | wc -l)
  NUM_ERRORS=$[ $NUM_ERRORS + $ne ]
  if [ "$1" == "" ]; then
    EXIT_STATUS=0
  else
    EXIT_STATUS=$1
  fi
  if [ $ne -lt 1 -a $EXIT_STATUS == 0 ]; then
    check_successful "Compilation OK."
  else
    if [ $EXIT_STATUS != 0 ]; then
      NUM_ERRORS=$[ $NUM_ERRORS + 1 ]
    fi
    err_report
    check_failed
  fi
}

function check_answer {
  local ANSWER=$(grep "answer=" $BOXOUT | cut -d = -f 2)
  if [ "$ANSWER" == "$1" ]; then
    check_successful "Result OK."
  else
    NUM_ERRORS=$[ $NUM_ERRORS + 1 ]
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
if [ -x $BOXEXEC ]; then
  BOX="$BOXEXEC $BOXOPTST"
  BOXLG="$BOXEXEC $BOXOPTLG $BOXOPTST"
else
  echo "I cannot find the executable file '$BOXEXEC'!"
  exit 1
fi
