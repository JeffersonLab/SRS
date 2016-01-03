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

TIPCIE_PATH		?= ${HOME}/work/TIpcie

CROSS_COMPILE		=
CC			= $(CROSS_COMPILE)gcc34
AR                      = ar
RANLIB                  = ranlib
INCS			= -I. -I../ -I${TIPCIE_PATH}
CFLAGS			= -Wall -g ${INCS} \
			  -L. -L../ -L${TIPCIE_PATH}

PROGS			= srsLibTest srsReadoutTest_TIpcie
SRC			= $(PROGS:%=%.c)
DEPS			= $(SRC:%.c=%.d)

all: $(PROGS)

clean distclean:
	@rm -f $(PROGS) $(DEPS) *~ *.so

%: %.c
	@echo ""
	@echo "Building $@ from $<"
	$(CC) $(CFLAGS) -o $@ $(@:%=%.c) $(LIBS_$@) -lrt -lsrs -lTIpcie

%.d: %.c
	@echo ""
	@echo "Building $@ from $<"
	@set -e; rm -f $@; \
	$(CC) -MM -shared $(INCS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

-include $(DEPS)

.PHONY: all clean distclean
