.PHONY: all svn_version html

all: html

svn_version:
	echo ".. |svnversion| replace:: "`svnversion -n` > svn_version.txt

html: svn_version
	mkdir -p html
	rst2html --strict --stylesheet-path=html/voidspace.css \
	         --link-stylesheet -v manual.txt html/manual.html \
                 -r 1 -g -t -d -s

