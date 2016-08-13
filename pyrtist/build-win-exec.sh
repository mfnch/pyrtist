#!/bin/bash
PYTHON=/cygdrive/c/Programmi/PyGTK/Python/python

$PYTHON setup.py py2exe

mkdir -p my-dist && cd my-dist
if [ ! -d boxer ]; then
  echo "You should copy the previous dist inside the directory my-dist"
  echo "and rename it with 'mv boxer-X.Y boxer'"
  exit 1
fi

cd boxer || (echo "Cannot find directory 'boxer'" && exit 1)
mv bin bin-old && mv ../../dist bin && \
cd bin-old && mv box examples glade ../bin && cd .. && rm -rf bin-old
