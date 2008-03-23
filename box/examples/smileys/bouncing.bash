#!/bin/bash

function cmnd {
  echo "$*"
  $*
}

BOX_FILE="bouncing.box"
TMP_DIR="./bouncing"
TMP_FILE="tmp.box"

mkdir -p $TMP_DIR
cd $TMP_DIR

FILE_LIST=""
for (( i = 0; i < 120; i += 1 )); do
  cat << EOF > $TMP_FILE
  t = 0.025*$i
EOF
  cat ../$BOX_FILE >> $TMP_FILE

  cmnd box -l g $TMP_FILE
  NEW_FILE="bouncing_$i.gif"
  cmnd convert bouncing.bmp -scale 80x80 $NEW_FILE
  FILE_LIST="$FILE_LIST $NEW_FILE"
done

cmnd gifsicle -l0 --delay 1 $FILE_LIST --output=../bouncing.gif
cd ..

