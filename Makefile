LIBSRCS = drvrEcdr814.c regNodeOps.c dirOps.c bitMenu.c ecDb.c  ecdr814Lowlevel.c ecLookup.c
SRCS = drvrTst.c genHeaders.c $(LIBSRCS) genMenuHdr.c

OBJS = $(SRCS:%.c=%.o)

LIBOBJS = $(LIBSRCS:%.c=%.o)

CC = gcc

CFLAGS=-g -O2 -DDIRSHELL -DDEBUG
LDFLAGS=-g

SUBDIRS = O.host O.vxppc

CROSS_COMPILE = $(patsubst .%,%,$(suffix $(CURDIR)))-
ifeq "$(CROSS_COMPILE)" "host-"
	CROSS_COMPILE=
endif

all: subdirs

.PHONY: prebuild subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)

.all: drvrTst drvr.o

PREBUILD = genMenuHdrtool menuDefs.h genHeaderstool fastKeyDefs.h genDbdtool

prebuild: $(SUBDIRS:%=%/Makefile) $(PREBUILD)

$(SUBDIRS):  prebuild
	$(MAKE) -C $@ .all

$(filter %tool, $(PREBUILD)):
	$(MAKE) -C O.host $(@:%tool=%)

O.%: O.host

O.%/Makefile:
	mkdir -p $(@:%/Makefile=%)
	echo "include ../Makefile" > $@

O.host/%:
	$(MAKE) -C O.host $(@:O.host/%=%)

%.o: ../%.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -I.. -c -o $@ $<

drvrTst: drvrTst.o $(LIBOBJS)
	$(CROSS_COMPILE)$(CC) $(CFLAGS)  -o $@ $^

drvr.o:	$(LIBOBJS)
	$(CROSS_COMPILE)$(LD) $(LDFLAGS) -r -o $@ $^

fastKeyDefs.h: O.host/genHeaders
	if ! O.host/genHeaders -k > $@ ; then $(RM) $@; fi

menuDefs.h:	bitMenu.c  O.host/genMenuHdr
	echo '#ifndef BIT_MENU_HEADER_DEFS_H' > $@
	echo '#define BIT_MENU_HEADER_DEFS_H' >> $@
	echo '/* DONT EDIT THIS FILE, IT WAS AUTOMATICALLY GENERATED */' >>$@
	if  ! O.host/genMenuHdr  >> $@ || ! echo '#endif' >>$@  ; then $(RM) $@; fi

genMenuHdr: genMenuHdr.o bitMenu.o
	$(CC) -o $@ $^

genDbd: genDbd.o ecDb.o bitMenu.o
	$(CC) -o $@ $^

genHeaders: genHeaders.o ecDb.o
	$(CC) -o $@ $^


clean:
	$(RM) -r O.host O.tgt

allclean: clean
	$(RM) fastKeyDefs.h menuDefs.h

DEPSUBDS = $(SUBDIRS:%=%.depend)

.PHONY: $(DEPSUBDS)
depend: prebuild $(DEPSUBDS)
	

$(DEPSUBDS):
	$(MAKE) -C $(@:%.depend=%) .depend

.depend:
	gccmakedep -I.. $(SRCS:%=../%)

