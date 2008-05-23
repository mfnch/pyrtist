.PHONY: all svn_version html

BOX=$(HOME)/local/bin/box

all: html

svn_version:
	echo ".. |svnversion| replace:: "`svnversion -n .` > svn_version.txt

poly.png: poly.box
	$(BOX) -l g poly.box

html: svn_version poly.png
	mkdir -p html
	if [ -x rst2html ]; then RST2HTML=rst2html; else RST2HTML=rst2html.py; fi; \
	$$RST2HTML --strict --stylesheet-path=html/voidspace.css \
	           --link-stylesheet -v manual.txt html/manual.html \
	           -r 1 -g -t -d -s

