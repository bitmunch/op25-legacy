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
 * Publicly-accesible default constuctor function for op25_decoder_ff.
 */
op25_decoder_ff_sptr op25_make_decoder_ff();

/*
 * The op25_decoder block.
 */
class op25_decoder_ff : public gr_block
{
private:
   op25_decoder_ff();
public:
   const char *device_name() const;
   gr_msg_queue_sptr get_msgq() const;
   void set_msgq(gr_msg_queue_sptr msgq);
};
