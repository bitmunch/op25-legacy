#
# Copyright 2004,2005,2006 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
# 

include $(top_srcdir)/Makefile.common

# Install this stuff so that it ends up as the gnuradio.howto module
# This usually ends up at:
#   ${prefix}/lib/python${python_version}/site-packages/gnuradio

ourpythondir = $(grpythondir)
ourlibdir    = $(grpyexecdir)

INCLUDES = $(STD_DEFINES_AND_INCLUDES) $(PYTHON_CPPFLAGS)

SWIGPYTHONARGS = $(SWIGPYTHONFLAGS) $(SWIGGRFLAGS)

ALL_IFILES = 				\
	$(LOCAL_IFILES)			\
	$(NON_LOCAL_IFILES)		

NON_LOCAL_IFILES =			\
	$(GNURADIO_CORE_INCLUDEDIR)/swig/gnuradio.i


LOCAL_IFILES = 				\
	repeater.i				

# These files are built by SWIG.  The first is the C++ glue.
# The second is the python wrapper that loads the _howto shared library
# and knows how to call our extensions.

BUILT_SOURCES = 			\
	repeater.cc			\
	repeater.py

# This gets howto.py installed in the right place
ourpython_PYTHON =			\
	repeater.py

ourlib_LTLIBRARIES = _repeater.la

# These are the source files that go into the shared library
_repeater_la_SOURCES = 	repeater\
	repeater.cc			\
	repeater_fsk4_slicer_fb.cc	\
	repeater_s2v.cc			\
	repeater_p25_frame_assembler.cc	\
	repeater_pipe.cc		\
	repeater_ctcss_squelch_ff.cc	\
	repeater_squelch_base_ff.cc	\
	repeater_vocoder.cc		\
	repeater_gardner_costas_cc.cc	\
	repeater_tdetect_cc.cc		\
	cic_filter.cc			\
	repeater_chan_usrp.cc		\
	repeater_chan_usrp_rx.cc	\
	p25_framer.cc			\
	bch.cc				\
	rs.cc

# magic flags
_repeater_la_CXXFLAGS = -I ../../../imbe_vocoder/src/lib \
			-I ../../../blocks/src/lib
_repeater_la_LDFLAGS = $(NO_UNDEFINED) -module -avoid-version -L../../../imbe_vocoder/src/lib/ -limbe_vocoder

# link the library against some comon swig runtime code and the 
# c++ standard library
_repeater_la_LIBADD = 			\
	$(PYTHON_LDFLAGS)		\
	-lstdc++			\
	../../../blocks/src/lib/software_imbe_decoder.lo	\
	../../../blocks/src/lib/imbe_decoder.lo

repeater.cc repeater.py: repeater.i $(ALL_IFILES)
	$(SWIG) $(SWIGPYTHONARGS) -module repeater -o repeater.cc $<

# These headers get installed in ${prefix}/include/gnuradio
# grinclude_HEADERS =			


# These swig headers get installed in ${prefix}/include/gnuradio/swig
swiginclude_HEADERS = 			\
	$(LOCAL_IFILES)


MOSTLYCLEANFILES = $(BUILT_SOURCES) *.pyc

# Don't distribute output of swig
dist-hook:
	@for file in $(BUILT_SOURCES); do echo $(RM) $(distdir)/$$file; done
	@for file in $(BUILT_SOURCES); do $(RM) $(distdir)/$$file; done
