#!/bin/bash

GV=gv
BOX=../src/box
BOXLIBDIR=../libs/g

export BOX_LIBRARY_PATH=$BOXLIBDIT:$BOX_LIBRARY_PATH

function introduce {
  echo "Starting '"$1"', press a key to continue..."
  read
}

function inspect {
  $GV $1
}

rm -f *.eps

introduce "The wheatstone example"
$BOX -l g wheatstone.box
inspect wheatstone.eps

introduce "The snake example"
$BOX -l g snake.box
inspect snake.eps

introduce "The closed line example"
$BOX -l g closed_line.box
inspect closed_line.eps

introduce "The multivibrator example"
$BOX -l g multivibrator.box
inspect multivibrator.eps

introduce "The cycloid example"
$BOX -l g cycloid.box
inspect cycloid.eps

