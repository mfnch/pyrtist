#!/bin/bash

ANIM_DIR=../../box/examples/smileys
ANIMS="bouncing shy washing_machine losing_eyes machine"

for ANIM in $ANIMS; do
  GIF=$ANIM.gif
  GIF0="$ANIM"_0.gif

  if [ ! -e $GIF ]; then
    [ -e $ANIM_DIR/$GIF ] && ln -s $ANIM_DIR/$GIF .
  fi
  if [ ! -e $GIF0 ]; then
    [ -e $ANIM_DIR/$GIF0 ] && ln -s $ANIM_DIR/$GIF0 .
  fi

  if [ ! -e $GIF -o ! -e $GIF0 ]; then
    (cd $ANIM_DIR && bash $ANIM.bash)
    rm -f $GIF $GIF0
    ln -s $ANIM_DIR/$GIF .
    ln -s $ANIM_DIR/$GIF0 .
  fi
done

cp *.gif ../out

