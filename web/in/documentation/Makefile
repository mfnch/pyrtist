.PHONY: all

DOC_DIR=../../../box/doc
OUT_DOC_DIR=../../out/documentation

all:
	(cd $(DOC_DIR) && make)
	mkdir -p $(OUT_DOC_DIR)
	cp $(DOC_DIR)/html/* $(OUT_DOC_DIR)

