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
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with OP25; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Boston, MA
 * 02110-1301, USA.
 */

#ifndef INCLUDED_GR_APCO_P25_DECODER_F_H
#define INCLUDED_GR_APCO_P25_DECODER_F_H

#include <gr_block.h>
#include <gr_msg_queue.h>

typedef unsigned char dibit;

typedef boost::shared_ptr<class gr_apco_p25_decoder_f> gr_apco_p25_decoder_f_sptr;

/*
 * gr_apco_p25_decoder_f is a GNU Radio block for decoding APCO P25
 * signals. This file expects its input to be a stream of symbols
 * from an appropriate demodulator and is a signal sink that produces
 * no outputs.
 */
class gr_apco_p25_decoder_f : public gr_block
{
public:
   virtual ~gr_apco_p25_decoder_f();
   virtual void forecast(int nof_output_items, gr_vector_int &nof_inputs_required);
   virtual int general_work(int nof_output_items,
                            gr_vector_int& nof_input_items,
                            gr_vector_const_void_star& input_items,
                            gr_vector_void_star& output_items);
private:
   friend gr_apco_p25_decoder_f_sptr gr_make_apco_p25_decoder_f(); // expose class to public ctor
   gr_apco_p25_decoder_f();
private:
};

gr_apco_p25_decoder_f_sptr gr_make_apco_p25_decoder_f();

#endif /* INCLUDED_GR_APCO_P25_DECODER_F_H */
