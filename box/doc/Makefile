.PHONY: all svn_version html

BOX=$(HOME)/local/bin/box
RST2HTML=python rst2html.py

all: html

svn_version:
	echo ".. |svnversion| replace:: "`svnversion -n .` > svn_version.txt

html/box-logo.png: box-logo.box
	$(BOX) -l g box-logo.box

html/window1.png: window.box
	$(BOX) -l g window.box

window.txt: window.txt.in window.box html/window1.png
	python subst.py window.box window.txt.in

pointlist.out: pointlist.box
	$(BOX) -l g pointlist.box | tail -n2 > pointlist.out

pointlist.txt: pointlist.txt.in pointlist.box pointlist.out
	python subst.py pointlist.box pointlist.txt.in

color.txt: color.txt.in color.box
	python subst.py color.box color.txt.in

html/line1.png: line.box
	$(BOX) -l g line.box

line.txt: line.txt.in html/line1.png
	python subst.py line.box line.txt.in

html/poly1.png: poly.box
	$(BOX) -l g poly.box

poly.txt: poly.txt.in html/poly1.png
	python subst.py poly.box poly.txt.in

circle.txt: circle.txt.in
	python subst.py circle.box circle.txt.in

gradient.txt:
	python subst.py gradient.box gradient.txt.in

style.txt:
	python subst.py style.box style.txt.in

html: svn_version html/box-logo.png color.txt pointlist.txt line.txt poly.txt \
      circle.txt style.txt gradient.txt window.txt installation.txt
	mkdir -p html
	$(RST2HTML) --strict --stylesheet-path=html/style.css \
	            --link-stylesheet -v manual.txt html/manual.html \
	            -r 1 -g -t -d -s

inst-html: installation.txt
	mkdir -p inst-html
	$(RST2HTML) --strict --stylesheet-path=inst-html/style.css \
	            --link-stylesheet -v installation.txt inst-html/installation.html \
	            -r 1 -g -t -d -s

