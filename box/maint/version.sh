#!/bin/sh

# File containing all the relased versions
VERSIONS=VERSIONS

DATE=`date +"%b %d %Y, %H:%M"`

# First let's read the latest version
if [ -e $VERSIONS ]; then
  LATEST_ALL=`tail -n1 $VERSIONS`
  LATEST_V=`echo $LATEST_ALL | cut -d , -f 1`
  OLD_DATE=`echo $LATEST_ALL | cut -d , -f 2-`
  MAJOR=`echo $LATEST_V | cut -d . -f 1`
  MINOR=`echo $LATEST_V | cut -d . -f 2`
  MAINT=`echo $LATEST_V | cut -d . -f 3`

else
  MAJOR="0"
  MINOR="0"
  MAINT="0"
  OLD_DATE="$DATE"
fi

OLD_MAJOR=$MAJOR
OLD_MINOR=$MINOR
OLD_MAINT=$MAINT
OLD_VERSION="$MAJOR.$MINOR.$MAINT"
CHANGED=0
for OPT in $*; do
  case $OPT in
  --new-major) MAJOR=$[ $MAJOR + 1 ]; MINOR="0"; MAINT="0"; CHANGED=1;;
  --new-minor) MINOR=$[ $MINOR + 1 ]; MAINT="0"; CHANGED=1;;
  --new-maint) MAINT=$[ $MAINT + 1 ]; CHANGED=1;;
  --current) echo $OLD_VERSION; exit 0;;
  --cur-major) echo $OLD_MAJOR; exit 0;;
  --cur-minor) echo $OLD_MINOR; exit 0;;
  --cur-maint) echo $OLD_MAINT; exit 0;;
  --cur-date) echo $OLD_DATE; exit 0;;
  --help) ;;
  *) echo "Unrecognized option"; exit 1;;
  esac
done

if [ $CHANGED -eq 1 ]; then
  VERSION="$MAJOR.$MINOR.$MAINT"
  echo "Creating new version: $OLD_VERSION ====> $VERSION"
  echo "$VERSION, $DATE" >> $VERSIONS
  sh ./adjust_configure_ac.sh

else
  echo "Box versioning system (to be used only by Box developers to release"
  echo "a new version of the program). This script increases the version"
  echo "number as specified from the command line. Here are the possible"
  echo "usages:"
  echo
  echo "./version.sh --new-maint  # for a maintanance release"
  echo "./version.sh --new-minor  # for a minor release"
  echo "./version.sh --new-major  # for a major release"
  echo "./version.sh --current    # print the current version"
  echo "./version.sh --cur-major  # print the XX of version XX.YY.ZZ"
  echo "./version.sh --cur-minor  # print the YY of version XX.YY.ZZ"
  echo "./version.sh --cur-maint  # print the ZZ of version XX.YY.ZZ"
  echo "./version.sh --cur-date   # print the date for the current version"
fi
