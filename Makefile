#
# File:
#    Makefile
#
# Description:
#    Makefile for the JLab TIpcie Trigger Interface module
#    using PC running Linux
#
# Uncomment DEBUG line, to include some debugging info ( -g and -Wall)
DEBUG=1
#
#
BASENAME=srs
ARCH=`uname -m`

CC			= gcc34
AR                      = ar
RANLIB                  = ranlib
CFLAGS			= -L. -L${LINUXVME_LIB}
INCS			= -I. -I${LINUXVME_INC}

LIBS			= lib${BASENAME}.a

ifdef DEBUG
CFLAGS			+= -Wall -g
else
CFLAGS			+= -O2
endif
SRC			= ${BASENAME}Lib.c
OBJ			= ${BASENAME}Lib.o
DEPS			= $(SRC:.c=.d)

all: echoarch $(LIBS)

%.o: %.c
	@echo
	@echo "Building $@ from $<"
	$(CC) $(CFLAGS) $(INCS) -c -o $@ $(SRC)

$(LIBS): $(OBJ)
	@echo
	@echo "Building $@ from $<"
	$(CC) -fpic -shared $(CFLAGS) $(INCS) -o $(@:%.a=%.so) $(SRC)
	$(AR) ruv $@ $<
	$(RANLIB) $@

%.d: %.c
	@echo
	@echo "Building $@ from $<"
	@set -e; rm -f $@; \
	$(CC) -MM -shared $(INCS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(DEPS)

clean:
	@rm -vf ${BASENAME}Lib.{o,d} lib${BASENAME}.{a,so}

echoarch:
	@echo "Make for $(ARCH)"

.PHONY: clean echoarch
