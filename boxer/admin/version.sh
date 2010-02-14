#!/bin/bash

# File containing all the relased versions
VERSIONS=VERSIONS

function show_help {
  echo "Boxer version information."
  echo
  echo "USAGE:"
  echo "./version.sh --current    # print the current version"
  echo "./version.sh --cur-major  # print the XX of version XX.YY.ZZ"
  echo "./version.sh --cur-minor  # print the YY of version XX.YY.ZZ"
  echo "./version.sh --cur-maint  # print the ZZ of version XX.YY.ZZ"
  echo "./version.sh --cur-date   # print the date for the current version"
}

# First let's read the latest version
if [ -e $VERSIONS ]; then
  LATEST_ALL=`tail -n1 $VERSIONS`
  LATEST_V=`echo $LATEST_ALL | cut -d / -f 1`
  DATE=`echo $LATEST_ALL | cut -d / -f 2`
  MAJOR=`echo $LATEST_V | cut -d . -f 1`
  MINOR=`echo $LATEST_V | cut -d . -f 2`
  MAINT=`echo $LATEST_V | cut -d . -f 3`

else
  MAJOR="0"
  MINOR="0"
  MAINT="0"
  DATE="Unknown"
fi

VERSION="$MAJOR.$MINOR.$MAINT"

for OPT in $*; do
  case $OPT in
  --current) echo $VERSION; exit 0;;
  --cur-major) echo $MAJOR; exit 0;;
  --cur-minor) echo $MINOR; exit 0;;
  --cur-maint) echo $MAINT; exit 0;;
  --cur-date) echo $DATE; exit 0;;
  --help) show_help; exit 0;;
  *) echo "Unrecognized option"; exit 1;;
  esac
done

show_help

