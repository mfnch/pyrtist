#!/bin/sh
# This script adjusts the version number in the Boxer sources. 
# After the execution of this script, the sources of Boxer will be consistent
# with the data stored in the VERSIONS file (the last line).

SETUP_PY="../setup.py"
BKP_SETUP_PY="$SETUP_PY~"

INFO_PY="../src/lib/info.py"
BKP_INFO_PY="$INFO_PY~"

CUR_VERSION="`./version.sh --current`"
CUR_DATE="`./version.sh --cur-date`"
CUR_MAJOR="`./version.sh --cur-major`"
CUR_MINOR="`./version.sh --cur-minor`"
CUR_MAINT="`./version.sh --cur-maint`"

# Adjust setup.py
cp $SETUP_PY $BKP_SETUP_PY
sed \
  -e "s@version[ \t]*=[ \t]*[\'][^\']*[\'][ \t]*@version = \'$CUR_VERSION\'@" \
  $BKP_SETUP_PY > $SETUP_PY

echo "Updated $SETUP_PY to be consistent with the VERSIONS file"
echo "A backup of the old file has been created in $BKP_SETUP_PY"

# Adjust src/info.py
cp $INFO_PY $BKP_INFO_PY
sed \
  -e "s@version[ \t]*=[^\n]*@version = ($CUR_MAJOR, $CUR_MINOR, $CUR_MAINT)@" \
  $BKP_INFO_PY > $INFO_PY

echo "Updated $INFO_PY to be consistent with the VERSIONS file"
echo "A backup of the old file has been created in $BKP_INFO_PY"

