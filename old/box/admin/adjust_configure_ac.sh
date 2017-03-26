#!/bin/sh
# This script adjusts the version number in the AC_INIT macro of the Box
# configure.ac file. After the execution of this script, the version
# in the AC_INIT macro will be consistent with the VERSIONS file.

CONFIGURE_AC="../configure.ac"
OLD_CONFIGURE_AC="$CONFIGURE_AC~"
cp $CONFIGURE_AC $OLD_CONFIGURE_AC
AC_INIT_LINE="`grep AC_INIT $OLD_CONFIGURE_AC`"
LEFT_PART="`echo $AC_INIT_LINE | cut -d , -f 1`"
CENTRAL_PART="`echo $AC_INIT_LINE | cut -d , -f 2`"
RIGHT_PART="`echo $AC_INIT_LINE | cut -d , -f 3-`"
CUR_VERSION="`./version.sh --current`"
CUR_DATE="`./version.sh --cur-date`"
CUR_MAJOR="`./version.sh --cur-major`"
CUR_MINOR="`./version.sh --cur-minor`"
CUR_MAINT="`./version.sh --cur-maint`"

NEW_AC_INIT_LINE="$LEFT_PART,[$CUR_VERSION],$RIGHT_PART"
sed \
  -e "s/AC_INIT([^)]*[)]/$NEW_AC_INIT_LINE/g" \
  -e "s@BOX_RELEASE_DATE[ \t]*=[^\n]*@BOX_RELEASE_DATE=\"$CUR_DATE\"@" \
  -e "s@BOX_VER_MAJOR[ \t]*=[^\n]*@BOX_VER_MAJOR=\"$CUR_MAJOR\"@" \
  -e "s@BOX_VER_MINOR[ \t]*=[^\n]*@BOX_VER_MINOR=\"$CUR_MINOR\"@" \
  -e "s@BOX_VER_MAINT[ \t]*=[^\n]*@BOX_VER_MAINT=\"$CUR_MAINT\"@" \
  $OLD_CONFIGURE_AC > $CONFIGURE_AC

echo "Updated configure.ac to be consistent with the new version"
echo "A backup of the old file has been created in $OLD_CONFIGURE_AC"
