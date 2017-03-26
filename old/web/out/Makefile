.PHONY: transfer tar

FILES=index.html rest2web.css titlebar.png example.html language.html

tar:
	tar cjvf ../web.tar.bz2 *
	scp ../web.tar.bz2 fnch@shell.sourceforge.net:/home/groups/b/bo/boxc/
	rm ../web.tar.bz2
 
transfer:
	scp $(FILES) fnch@shell.sourceforge.net:/home/groups/b/bo/boxc/htdocs

