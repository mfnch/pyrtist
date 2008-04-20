#!/bin/bash

function cmnd {
  echo "$*"
  $*
}

BOX_FILE="losing_eyes.box"
TMP_DIR="./uneyed"
TMP_FILE="tmp.box"

mkdir -p $TMP_DIR
cd $TMP_DIR

FILE_LIST=""
for (( i = 0; i < 200; i += 3 )); do
  cat << EOF > $TMP_FILE
  t = 0.06*$i
EOF
  cat ../$BOX_FILE >> $TMP_FILE

  cmnd box -l g $TMP_FILE
  NEW_FILE="smiley_$i.gif"
  cmnd convert smiley.bmp -scale 69x69 $NEW_FILE
  FILE_LIST="$FILE_LIST $NEW_FILE"
done

cmnd gifsicle -l0 --delay 1 $FILE_LIST --output=../losing_eyes.gif
cp smiley_0.gif ../losing_eyes_0.gif
cd ..

