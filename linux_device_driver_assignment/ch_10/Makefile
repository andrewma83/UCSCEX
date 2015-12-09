OBJ1 := main
OBJ2 := proc
OBJ3 := device
OBJ4 := myps
OBJ5 := interrupt_handler
LDDM := CDDAMA

#  create one .o .. from multiple .o files
#  .. do not need the .ko rule 
#  .. the kernel will auto create .ko from the obj-m line below.
obj-m := $(LDDM).o
$(LDDM)-objs := \
    $(OBJ1).o \
    $(OBJ2).o \
    $(OBJ3).o \
    $(OBJ4).o \
    $(OBJ5).o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

LDD := /lib/modules/$(shell uname -r)/kernel/drivers/ldd/

all: compile CDD2app

compile:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

###
###  Alternatively, you may want to use the early 2.6 syntax of
###  $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
###

CDD2app:
	-gcc -o CDD2app CDD2app.c;

clean: 
	-@rm -fr \
	    $(LDDM).* \
	    .$(LDDM).* \
	    $(OBJ1).*o \
	    $(OBJ2).*o \
	    $(OBJ3).*o \
	    $(OBJ4).*o \
	    $(OBJ5).*o \
	    $(OBJ1).*.* \
	    .$(OBJ1).* \
	    $(OBJ2).*.* \
	    .$(OBJ2).* \
	    $(OBJ3).*.* \
	    .$(OBJ3).* \
	    $(OBJ4).*.* \
	    .$(OBJ4).* \
	    $(OBJ5).*.* \
	    .$(OBJ5).* \
	    CDD2app \
	    .tmp_versions* [mM]odule*

mem:
	ls -l /dev | egrep "^c" | egrep "[ 	][ 	]*1," | sort +5.0n -6.0
