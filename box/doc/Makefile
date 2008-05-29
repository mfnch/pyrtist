.PHONY: all svn_version html

BOX=$(HOME)/local/bin/box
RST2HTML=python rst2html.py

all: html

svn_version:
	echo ".. |svnversion| replace:: "`svnversion -n .` > svn_version.txt

html/box-logo.png: box-logo.box
	$(BOX) -l g box-logo.box

html/poly1.png: poly.box
	$(BOX) -l g poly.box

poly.txt: poly.txt.in
	python subst.py poly.box poly.txt.in

html: svn_version html/box-logo.png html/poly1.png poly.txt
	mkdir -p html
	$(RST2HTML) --strict --stylesheet-path=html/style.css \
	            --link-stylesheet -v manual.txt html/manual.html \
	            -r 1 -g -t -d -s

