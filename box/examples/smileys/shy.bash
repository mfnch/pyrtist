#!/bin/bash

function cmnd {
  echo "$*"
  $*
}

BOX_FILE="shy.box"
TMP_DIR="./shy"
TMP_FILE="tmp.box"

mkdir -p $TMP_DIR
cd $TMP_DIR

FILE_LIST=""
for (( i = -12; i <= 32; i += 4 )); do
  cat << EOF > $TMP_FILE
  x = 0.02*$i
EOF
  cat ../$BOX_FILE >> $TMP_FILE

  cmnd box -l g $TMP_FILE
  NEW_FILE="smiley_$i.gif"
  cmnd convert smiley.bmp -scale 69x69 $NEW_FILE
  FILE_LIST="$FILE_LIST $NEW_FILE"
done

cmnd gifsicle -l0 --delay 1 $FILE_LIST --output=../shy.gif
cp smiley_0.gif ../shy_0.gif
cd ..

