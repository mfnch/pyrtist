.PHONY: all svn_version html

BOX=$(HOME)/local/bin/box
RST2HTML=python rst2html.py

all: html

svn_version:
	echo ".. |svnversion| replace:: "`svnversion -n .` > svn_version.txt

poly.png: poly.box
	$(BOX) -l g poly.box

html: svn_version poly.png
	mkdir -p html
	$(RST2HTML) --strict --stylesheet-path=html/style.css \
	            --link-stylesheet -v manual.txt html/manual.html \
	            -r 1 -g -t -d -s

