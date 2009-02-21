#!/bin/bash

# File containing all the relased versions
VERSIONS=VERSIONS

DATE=`date +"%b %d %Y, %H:%M:%S"`

# First let's read the latest version
if [ -e $VERSIONS ]; then
  LATEST_ALL=`tail -n1 $VERSIONS`
  LATEST_V=`echo $LATEST_ALL | cut -d / -f 1`
  OLD_DATE=`echo $LATEST_ALL | cut -d / -f 2`
  OLD_BOXCORE_VERSION=`echo $LATEST_ALL | cut -d / -f 3`
  MAJOR=`echo $LATEST_V | cut -d . -f 1`
  MINOR=`echo $LATEST_V | cut -d . -f 2`
  MAINT=`echo $LATEST_V | cut -d . -f 3`
  BC_CURRENT=`echo $OLD_BOXCORE_VERSION | cut -d . -f 1`
  BC_REVISION=`echo $OLD_BOXCORE_VERSION | cut -d . -f 2`
  BC_AGE=`echo $OLD_BOXCORE_VERSION | cut -d . -f 3`

else
  MAJOR="0"
  MINOR="0"
  MAINT="0"
  OLD_DATE="$DATE"
  BC_CURRENT="0"
  BC_REVISION="0"
  BC_AGE="0"
fi

OLD_MAJOR=$MAJOR
OLD_MINOR=$MINOR
OLD_MAINT=$MAINT
OLD_VERSION="$MAJOR.$MINOR.$MAINT"
CHANGED=0
BOXCORE_CHANGE="none"
IS_PATCH="yes"
for OPT in $*; do
  case $OPT in
  --new-major) MAJOR=$[ $MAJOR + 1 ]; MINOR="0"; MAINT="0"; CHANGED=1
               IS_PATCH="no";;
  --new-minor) MINOR=$[ $MINOR + 1 ]; MAINT="0"; CHANGED=1; IS_PATCH="no";;
  --new-maint) MAINT=$[ $MAINT + 1 ]; CHANGED=1;;
  --new-bc-fix) BOXCORE_CHANGE="patch";;
  --new-bc-ext) BOXCORE_CHANGE="extension";;
  --new-bc-int) BOXCORE_CHANGE="interface";;
  --current) echo $OLD_VERSION; exit 0;;
  --cur-major) echo $OLD_MAJOR; exit 0;;
  --cur-minor) echo $OLD_MINOR; exit 0;;
  --cur-maint) echo $OLD_MAINT; exit 0;;
  --cur-date) echo $OLD_DATE; exit 0;;
  --get-bc-cur) echo $BC_CURRENT; exit 0;;
  --get-bc-rev) echo $BC_REVISION; exit 0;;
  --get-bc-age) echo $BC_AGE; exit 0;;
  --help) ;;
  *) echo "Unrecognized option"; exit 1;;
  esac
done

if [ $IS_PATCH == "yes" ]; then
  case $BOXCORE_CHANGE in
  patch) BC_REVISION=$[ $BC_REVISION + 1 ];;
  extension) BC_CURRENT=$[ $BC_CURRENT + 1 ]; BC_REVISION="0"
                BC_AGE=$[ $BC_AGE + 1 ];;
  interface) BC_CURRENT=$[ $BC_CURRENT + 1 ]; BC_REVISION="0"; BC_AGE="0";;
  esac

else
  if [ $BOXCORE_CHANGE != "none" ]; then
    echo "ERROR: a new minor/major release of Box requires the change" \
         "of the libboxcore name, which is then a totally new library" \
         "You are then not expected to provide any --new-bc-* option!"
    exit 1
  fi

  BC_CURRENT="0"
  BC_REVISION="0"
  BC_AGE="0"
fi

if [ $CHANGED -eq 1 ]; then
  VERSION="$MAJOR.$MINOR.$MAINT"
  BOXCORE_VERSION="$BC_CURRENT.$BC_REVISION.$BC_AGE"
  echo "Creating new version: $OLD_VERSION ====> $VERSION"
  echo "$VERSION/$DATE/$BOXCORE_VERSION" >> $VERSIONS
  sh ./adjust_configure_ac.sh
  sh ./adjust_makefile_am.sh

else
  echo "Box versioning system (to be used only by Box developers to release"
  echo "a new version of the program). This script increases the version"
  echo "number as specified from the command line. Here are the possible"
  echo "usages:"
  echo
  echo "./version.sh --new-maint  # for a maintanance release"
  echo "./version.sh --new-minor  # for a minor release"
  echo "./version.sh --new-major  # for a major release"
  echo "./version.sh --new-bc-fix # no interface changes in libboxcore"
  echo "./version.sh --new-bc-ext # compatible extension of libboxcore"
  echo "./version.sh --new-bc-int # new incompatible version of libboxcore"
  echo "./version.sh --current    # print the current version"
  echo "./version.sh --cur-major  # print the XX of version XX.YY.ZZ"
  echo "./version.sh --cur-minor  # print the YY of version XX.YY.ZZ"
  echo "./version.sh --cur-maint  # print the ZZ of version XX.YY.ZZ"
  echo "./version.sh --cur-date   # print the date for the current version"
fi
