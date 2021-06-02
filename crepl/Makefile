NAME := $(shell basename $(PWD))
export MODULE := M4
all: $(NAME)-64 $(NAME)-32
LDFLAGS += -ldl

rm:
	rm tmp_c_* tmp_so_*

include ../Makefile
