
empty =
space = $(empty) $(empty)

#-------------------------------------------------------------------------------
# Opzioni al compilatore
OPTIONS = -Wall -g # -ansi -pedantic -DDEBUG

# Directory "madre"
TOP = ./

# Directories in cui cercare i file di include (*.h)
INCL_DIRS = common/ compiler/ virtmach/ virtmach/vlib/ graphic/

# Directory che contiene i file sorgenti comuni
COMN_DIRS = common/

# Directory che contiene i sorgenti del compilatore
CMPL_DIRS = compiler/

# Directory che contiene i sorgenti della macchina virtuale
VRMC_DIRS = virtmach/ virtmach/vlib

# Directory che contiene i sorgenti della libreria grafica
GRAP_DIRS = graphic/

# Files oggetto
OBJECTS = messages.o array.o str.o \
 symbols.o main.o registers.o typeman.o compiler.o parser.o \
 virtmach.o builtins.o

# opcodes.o \
# fig.o bm1bit.o bm4bit.o ps.o bmcommon.o graphic.o rasterizer.o grpline.o \
# settings.o simplefn.o units.o window.o lists.o plist.o flist.o print.o line.o \
# circle.o poly.o put.o autoput.o


#-------------------------------------------------------------------------------
# Directories dei sorgenti
SRCS_DIRS = $(COMN_DIRS) $(CMPL_DIRS) $(VRMC_DIRS) $(GRAP_DIRS)

VPATH = $(subst $(space),:,$(SRCS_DIRS))

INCL_DIRS := $(patsubst %,-I%,$(INCL_DIRS))

#-------------------------------------------------------------------------------
box: $(OBJECTS)
	gcc -o $@ -lm $(OBJECTS)

# Compilo main.c

main.o: main.c types.h messages.h defaults.h
	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

# Compilo i sorgenti comuni
#
messages.o: messages.c types.h str.h messages.h defaults.h
	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

array.o: array.c types.h debug.h messages.h array.h
	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

str.o: str.c types.h messages.h str.h
	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

# Uso lex e yacc per costruire i file del compilatore
# Lex ammette -d per il debugging
lex.yy.c: tokenizer.lex.c tokenizer.h types.h messages.h array.h str.h compiler.h defaults.h
	lex -8 $<

# Yacc ammette -t per il debugging
y.tab.c: parser.yacc.c lex.yy.c tokenizer.h types.h messages.h array.h str.h virtmach.h compiler.h defaults.h
	yacc -t -v $<

# Compilo i sorgenti del compilatore di box
#
parser.o: y.tab.c
	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

typeman.o: typeman.c types.h defaults.h messages.h array.h virtmach.h registers.h compiler.h
	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

compiler.o: compiler.c types.h defaults.h messages.h array.h virtmach.h registers.h compiler.h structure.c
	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

registers.o: registers.c types.h defaults.h messages.h array.h
	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

symbols.o: symbols.c types.h messages.h str.h array.h defaults.h
	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

builtins.o: builtins.c types.h defaults.h messages.h array.h str.h virtmach.h registers.h compiler.h
	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

# Compilo i sorgenti della macchina virtuale
#
virtmach.o: virtmach.c virtmach.h vmexec.c types.h defaults.h messages.h array.h
	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#opcodes.o: opcodes.c opcodes.h buffer.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

# Compilo i sorgenti che gestiscono la grafica
#fig.o: fig.c error.h buffer.h graphic.h fig.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#bm1bit.o: bm1bit.c error.h graphic.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#bm4bit.o: bm4bit.c error.h graphic.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#ps.o: ps.c error.h graphic.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#bmcommon.o: bmcommon.c error.h graphic.h bitmap.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#graphic.o: graphic.c error.h graphic.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#rasterizer.o: rasterizer.c error.h graphic.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#grpline.o: grpline.c error.h graphic.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

# Compilo i sorgenti della libreria della macchina virtuale
#simplefn.o: simplefn.c types.h vlib.h graphic.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#units.o: units.c types.h vlib.h graphic.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#window.o: window.c types.h vlib.h graphic.h str.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#lists.o: lists.c lists.h error.h types.h buffer.h vlib.h debug.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#plist.o: plist.c plist.h lists.h error.h types.h buffer.h vlib.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#flist.o: flist.c lists.h error.h types.h vlib.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#print.o: print.c types.h vlib.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#line.o: line.c debug.h types.h error.h vlib.h lists.h plist.h graphic.h settings.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#circle.o: circle.c debug.h types.h error.h vlib.h graphic.h settings.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#poly.o: poly.c debug.h types.h error.h vlib.h graphic.h settings.h lists.h plist.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#put.o: put.c debug.h types.h error.h vlib.h graphic.h settings.h lists.h plist.h fig.h autoput.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#autoput.o: autoput.c debug.h error.h autoput.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

#settings.o: settings.c settings.h
#	gcc $(OPTIONS) -c $(INCL_DIRS) $< -o $@

.PHONY: clean
clean:
	rm -f box lex.yy.c y.tab.c y.output $(OBJECTS)
	find ./ -name "*~" -exec rm -f {} \;

package: clean
	D=$$(date +"%d%b%Y") && cd .. && tar cjvf box_$$D.tar.bz2 box/*

test:
	cd tests && bash scope.bash && bash structure.bash && bash bugs.bash
