.PHONY: all svn_version html

BOX=$(HOME)/local/bin/box
RST2HTML=python rst2html.py

all: html

svn_version:
	echo ".. |svnversion| replace:: "`svnversion -n .` > svn_version.txt

html/box-logo.png: box-logo.box
	$(BOX) -l g box-logo.box

html/pointlist1.png: pointlist.box
	$(BOX) -l g pointlist.box

pointlist.txt: pointlist.txt.in html/pointlist1.png
	python subst.py pointlist.box pointlist.txt.in

html/line1.png: line.box
	$(BOX) -l g line.box

line.txt: line.txt.in html/line1.png
	python subst.py line.box line.txt.in

html/poly1.png: poly.box
	$(BOX) -l g poly.box

poly.txt: poly.txt.in html/poly1.png
	python subst.py poly.box poly.txt.in

html: svn_version html/box-logo.png pointlist.txt line.txt poly.txt
	mkdir -p html
	$(RST2HTML) --strict --stylesheet-path=html/style.css \
	            --link-stylesheet -v manual.txt html/manual.html \
	            -r 1 -g -t -d -s

