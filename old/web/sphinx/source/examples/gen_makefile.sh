#!/bin/bash

function get_info {
  python -c 'execfile("'$1'"); print '$2
}

cd $1 || exit 1

echo ".PHONY: all clean"
echo

ALL_EXAMPLE_FILES=""
for EXAMPLE_FILE in *.example; do
  rst_out=`get_info $EXAMPLE_FILE rst_out`
  ALL_EXAMPLE_FILES="$ALL_EXAMPLE_FILES $rst_out"
done
echo "all: $ALL_EXAMPLE_FILES"
echo

for EXAMPLE_FILE in *.example; do
  rst_out=`get_info $EXAMPLE_FILE rst_out`
  box_source=`get_info $EXAMPLE_FILE box_source`
  echo "$rst_out: skeleton $EXAMPLE_FILE $box_source"
  echo "	python ../create_example.py $EXAMPLE_FILE"
  echo
done

echo "clean:"
echo "	rm -f *.png *.txt *.rst *.eps *.ps *.pdf *.bmp thumbnails.dat"
echo

