#!/bin/bash

function cmnd {
  echo "$*"
  $*
}

BOX_FILE="stt.box"
TMP_DIR="./stt"
TMP_FILE="tmp.box"

mkdir -p $TMP_DIR
cd $TMP_DIR

FILE_LIST=""
for (( i = 0; i < 40; i += 1 )); do
  cat << EOF > $TMP_FILE
  t = 0.05*$i
EOF
  cat ../$BOX_FILE >> $TMP_FILE

  cmnd box -l g $TMP_FILE
  NEW_FILE="stt_$i.gif"
  convert stt.png $NEW_FILE
  FILE_LIST="$FILE_LIST $NEW_FILE"
done

cmnd gifsicle -l0 -d20 $FILE_LIST --output=../stt.gif
cd ..

