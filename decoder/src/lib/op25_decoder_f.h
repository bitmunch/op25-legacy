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

#ifndef INCLUDED_OP25_DECODER_F_H
#define INCLUDED_OP25_DECODER_F_H

#include <data_unit.h>
#include <gr_sync_block.h>
#include <gr_msg_queue.h>

typedef boost::shared_ptr<class op25_decoder_f> op25_decoder_f_sptr;

op25_decoder_f_sptr op25_make_decoder_f(gr_msg_queue_sptr msgq);

/*
 * op25_decoder_f is a GNU Radio block for decoding APCO P25
 * signals. This file expects its input to be a stream of symbols from
 * the demodulator and is a signal sink that produces no outputs.
 * Processed messages are sent to the message queue.
 */
class op25_decoder_f : public gr_sync_block
{
public:
   virtual ~op25_decoder_f();
   virtual int work(int nof_output_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items);
private:
   friend op25_decoder_f_sptr op25_make_decoder_f(gr_msg_queue_sptr msgq); // expose class to public ctor
   op25_decoder_f(gr_msg_queue_sptr msgq);
   bool correlates(dibit d);
   bool identifies(dibit d);
   void receive_symbol(dibit d);
   void sync_receive_symbol(dibit d);
private:
   gr_msg_queue_sptr d_msgq;
   enum { SYNCHRONIZING, SYNCHRONIZED } d_state;
   enum { IDENTIFYING, READING } d_substate;
   uint64_t d_frame_sync;
   uint64_t d_network_ID;
   uint32_t d_symbol;
   data_unit_sptr d_data_unit;
   uint32_t d_data_units;
   uint32_t d_unrecognized;
};

#endif /* INCLUDED_OP25_DECODER_F_H */