#!/bin/bash

function cmnd {
  echo "$*"
  $*
}

BOX_FILE="washing_machine.box"
TMP_DIR="./washing"
TMP_FILE="tmp.box"

mkdir -p $TMP_DIR
cd $TMP_DIR

FILE_LIST=""
for (( i = 0; i < 100; i += 1 )); do
  cat << EOF > $TMP_FILE
  t = $i
EOF
  cat ../$BOX_FILE >> $TMP_FILE

  cmnd box -l g $TMP_FILE
  NEW_FILE="smiley_$i.gif"
  cmnd convert smiley.bmp -scale 69x69 $NEW_FILE
  FILE_LIST="$FILE_LIST $NEW_FILE"
done

cmnd gifsicle --delay 1 $FILE_LIST --output=../washing.gif
cd ..
