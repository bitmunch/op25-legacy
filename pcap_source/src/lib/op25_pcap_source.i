/* -*- C++ -*- */

%feature("autodoc", "1");

%include "exception.i"
%import "gnuradio.i"

%{
#include <boost/shared_ptr.hpp>
#include "gnuradio_swig_bug_workaround.h"
#include "op25_pcap_source.h"
%}

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 *
 * This does some behind-the-scenes magic so we can invoke
 * op25_make_pcap_source from python as op25.pcap_source.
 */
GR_SWIG_BLOCK_MAGIC(op25, pcap_source);

/*
 * Publicly-accesible constuctor function for op25_pcap_source.
 */
op25_pcap_source_sptr op25_make_pcap_source(const char *path, float delay, bool repeat);

/*
 * The op25_pcap_source block. Reads symbols from a tcpdump-formatted
 * file and produces a stream of symbols of the appropriate size.
 */
class op25_pcap_source : public gr_sync_block
{
private:
   op25_pcap_source(const char *path);
};
