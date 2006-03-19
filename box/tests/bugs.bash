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

echo ">>"
check_final
