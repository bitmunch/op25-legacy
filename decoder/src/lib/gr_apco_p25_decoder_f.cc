/* -*- C++ -*- */
/*
 * Copyright 2008 Steve Glass
 * 
 * This file is part of OP25.
 * 
 * OP25 is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * OP25 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with OP25; see the file COPYING. If not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Boston, MA
 * 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_apco_p25_decoder_f.h>
#include <gr_io_signature.h>

/*
 * The private constructor.
 */
gr_apco_p25_decoder_f::gr_apco_p25_decoder_f() :
   gr_block("apco_p25_decoder_f",
            gr_make_io_signature(1, 1, sizeof(float)),
            gr_make_io_signature(0, 0, sizeof(float)))
{
}

/*
 * Destruct an instance of this class.
 */
gr_apco_p25_decoder_f::~gr_apco_p25_decoder_f()
{
}

/*
 * Given output request for noutput_items return number of input items
 * required. This block doesn't produce an output stream so this just
 * returns its input.
 */
void
gr_apco_p25_decoder_f::forecast(int nof_output_items, gr_vector_int& nof_inputs_required)
{
    int items = nof_output_items;
    for(size_t i = 0; i < nof_inputs_required.size(); ++i)
       nof_inputs_required[i] = items;   
}

/*
 * Take an incoming float value, convert to a symbol and process.
 */
int  
gr_apco_p25_decoder_f::general_work(int nof_output_items,
                            gr_vector_int& nof_input_items,
                            gr_vector_const_void_star& input_items,
                            gr_vector_void_star& output_items)
{
   // const float *in = reinterpret_cast<const float*>(&input_items[0]);

   // ToDo: decode input

   consume_each(nof_output_items); // tell runtime how many items we consumed
   return 0; // tell runtime system how many items we produced.
}

/*
 * Create a new instance of gr_apco_p25_decoder_f and wrap it in a
 * shared_ptr. This is effectively the public constructor.
 */
gr_apco_p25_decoder_f_sptr
gr_make_apco_p25_decoder_f()
{
   return gr_apco_p25_decoder_f_sptr(new gr_apco_p25_decoder_f);
}
