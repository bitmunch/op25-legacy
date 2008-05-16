/* -*- C++ -*- */

%feature("autodoc", "1");

%include "exception.i"
%import "gnuradio.i"

%{
#include "gnuradio_swig_bug_workaround.h"
#include "gr_apco_p25_decoder_f.h"
#include <stdexcept>
%}

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 *
 * This does some behind-the-scenes magic so we can access
 * apco_p25_decoder_f from python as gr.apco_p25_decoder_f
 */
GR_SWIG_BLOCK_MAGIC(gr, apco_p25_decoder_f);

/*
 * Publically accesible constuctor function for apco_p25_decoder_f.
 */
gr_apco_p25_decoder_f_sptr gr_make_apco_p25_decoder_f();

/*
 * The actual gr_apco_p25_decoder block.
 */
class gr_apco_p25_decoder_f : public gr_sync_block
{
private:
   gr_apco_p25_decoder_f();
};
