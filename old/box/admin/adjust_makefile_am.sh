#!/bin/bash
# This script adjusts the version number in the AC_INIT macro of the Box
# configure.ac file. After the execution of this script, the version
# in the AC_INIT macro will be consistent with the VERSIONS file.

MAKEFILES=`find ../ -name "Makefile.am"`

CUR_MAJOR="`./version.sh --cur-major`"
CUR_MINOR="`./version.sh --cur-minor`"
BC_CUR=`./version.sh --get-bc-cur`
BC_REV=`./version.sh --get-bc-rev`
BC_AGE=`./version.sh --get-bc-age`

for MAKEFILE in $MAKEFILES; do
  C=`grep libboxcore $MAKEFILE`
  if [ x"$C" != x ]; then
    MAKEFILE_BKP="$MAKEFILE~"
    echo "Creating backup of $MAKEFILE in $MAKEFILE_BKP"
    cp $MAKEFILE $MAKEFILE_BKP
    OLD_VERSION_INFO="-version-info[ ]+[0-9]+:[0-9]+:[0-9]+"
    NEW_VERSION_INFO="-version-info $BC_CUR:$BC_REV:$BC_AGE"
    echo "Adjusting: $MAKEFILE"
    sed -r \
      -e "s/libboxcore[0-9]+[.][0-9]+/libboxcore$CUR_MAJOR.$CUR_MINOR/g" \
      -e "s/libboxcore[0-9]+_[0-9]+/libboxcore$CUR_MAJOR""_$CUR_MINOR/g" \
      -e "s/$OLD_VERSION_INFO/$NEW_VERSION_INFO/g" \
      $MAKEFILE_BKP > $MAKEFILE
  fi
done

