#!/usr/bin/make
#

CDEBUG = -g
CPPFLAGS = $(CDEBUG) $(CDEFS)
CXXFLAGS = -I.
LIBS = 
LDFLAGS = 

SRCFILES = *.cc
DEPFILES = .*.d
OBJFILES = *.o

SRCS = $(wildcard $(SRCFILES))
DEPS = $(SRCS:%.cc=.%.d)
OBJS = $(SRCS:%.cc=%.o)
AOUT = imbe

.PHONY: clean

.%.d: %.cc
	$(SHELL) -ec '$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; [ -s $@ ] || rm -f $@'

$(AOUT): $(LIBS) $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(AOUT) $(OBJFILES) $(DEPFILES) *~

include $(DEPS)
