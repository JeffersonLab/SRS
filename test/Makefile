#
# File:
#    Makefile
#
# Description:
#    Makefile for the JLAB extensions to the GEFANUC Controller API
#
#
# $Date$
# $Rev$
#

LINUXVME_LIB	?= ${CODA}/extensions/linuxvme/libs
LINUXVME_INC	?= ${CODA}/extensions/linuxvme/include

CROSS_COMPILE		=
CC			= $(CROSS_COMPILE)gcc
AR                      = ar
RANLIB                  = ranlib
INCS			= -I. -I../ -I${LINUXVME_INC}
CFLAGS			= -Wall -g ${INCS} \
			  -L. -L../ -L${LINUXVME_LIB} 

PROGS			= srsLibTest
SRC			= $(PROGS:%=%.c)
DEPS			= $(SRC:%.c=%.d)

all: $(PROGS)

clean distclean:
	@rm -f $(PROGS) $(DEPS) *~ *.so

%: %.c
	@echo ""
	@echo "Building $@ from $<"
	$(CC) $(CFLAGS) -o $@ $(@:%=%.c) $(LIBS_$@) -lrt -lsrs

%.d: %.c
	@echo ""
	@echo "Building $@ from $<"
	@set -e; rm -f $@; \
	$(CC) -MM -shared $(INCS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(DEPS)

.PHONY: all clean distclean
