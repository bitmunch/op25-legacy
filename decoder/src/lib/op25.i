/* -*- C++ -*- */

%feature("autodoc", "1");

%include "exception.i"
%import "gnuradio.i"

%{
#include "gnuradio_swig_bug_workaround.h"
#include "op25_decoder_ff.h"
#include <stdexcept>
%}

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 *
 * This does some behind-the-scenes magic so we can access
 * op25_decoder_ff from python as op25.decoder_ff.
 */
GR_SWIG_BLOCK_MAGIC(op25, decoder_ff);

/*
 * Publicly-accesible constuctor function for op25_decoder_ff.
 */
op25_decoder_ff_sptr op25_make_decoder_ff(gr_msg_queue_sptr msgq);

/*
 * The actual op25_decoder block.
 */
class op25_decoder_ff : public gr_block
{
public:
   uint32_t audio_rate() const;
private:
   op25_decoder_ff(gr_msg_queue_sptr msgq);
};
