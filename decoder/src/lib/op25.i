/* -*- C++ -*- */

%feature("autodoc", "1");

%include "exception.i"
%import "gnuradio.i"

%{
#include "gnuradio_swig_bug_workaround.h"
#include "op25_decoder_f.h"
#include <stdexcept>
%}

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 *
 * This does some behind-the-scenes magic so we can access
 * op25_decoder_f from python as op25.decoder_f
 */
GR_SWIG_BLOCK_MAGIC(op25,decoder_f);

/*
 * Publicly-accesible constuctor function for op25_decoder_f.
 */
op25_decoder_f_sptr op25_make_decoder_f(gr_msg_queue_sptr msgq);

/*
 * The actual op25_decoder block.
 */
class op25_decoder_f : public gr_sync_block
{
private:
   op25_decoder_f(gr_msg_queue_sptr msgq);
};
