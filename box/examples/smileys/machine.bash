#!/bin/bash

function cmnd {
  echo "$*"
  $*
}

BOX_FILE="machine.box"
TMP_DIR="./machine"
TMP_FILE="tmp.box"

mkdir -p $TMP_DIR
cd $TMP_DIR

FILE_LIST=""
for (( i = 0; i < 200; i += 4 )); do
  cat << EOF > $TMP_FILE
  wa = 0.1*$i
EOF
  cat ../$BOX_FILE >> $TMP_FILE

  cmnd box -l g $TMP_FILE
  NEW_FILE="pump_$i.gif"
  cmnd convert machine.bmp -scale 100x100 $NEW_FILE
  FILE_LIST="$FILE_LIST $NEW_FILE"
done

cmnd gifsicle -l0 -d1 $FILE_LIST --output=../machine.gif
cp pump_0.gif ../machine_0.gif
cd ..

