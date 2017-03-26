#!/bin/bash

. common.bash

BC_FILE="test.bvm"

function bytecode {
  $BOXEXEC $BOXOPTLG -t -o $2 $1 >$BOXOUT 2>&1
}

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

echo "<< Testing if the bytecode for some complex examples changed:"
rm -f $ERRORS

#------------------------------------------------------------------------------
EXAMPLES="../examples/wheatstone.box ../examples/multivibrator.box"

for EXAMPLE in $EXAMPLES; do
  SAVED_BC="`basename $EXAMPLE .box`.bvm"
  DIFF_BC="`basename $EXAMPLE .box`.diff"

  if [ -r $SAVED_BC ]; then
    test_next
    echo "Testing the bytecode for $EXAMPLE"
    bytecode $EXAMPLE $BC_FILE
    check_noerr
    diff $SAVED_BC $BC_FILE > $DIFF_BC
    if [ $? -eq 0 ]; then
      check_successful "no diffs found!"
      rm -f $DIFF_BC

    else
      check_failed "diffs saved in $DIFF_BC"
    fi

  else
    echo "Cannot find bytecode. Saving it into $SAVED_BC"
    bytecode $EXAMPLE $SAVED_BC
  fi
done

rm -f $BC_FILE

#------------------------------------------------------------------------------
echo ">>"
check_final
