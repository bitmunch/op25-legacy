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

#ifndef INCLUDED_OP25_DECODER_FF_H
#define INCLUDED_OP25_DECODER_FF_H

#include <bitset>
#include <data_unit.h>
#include <gr_block.h>
#include <gr_msg_queue.h>
#include <imbe_decoder.h>

typedef boost::shared_ptr<class op25_decoder_ff> op25_decoder_ff_sptr;

op25_decoder_ff_sptr op25_make_decoder_ff(gr_msg_queue_sptr msgq);

typedef std::bitset<48> frame_sync;

/**
 * op25_decoder_ff is a GNU Radio block for decoding APCO P25
 * signals. This class expects its input to be a stream of dibit
 * symbols from the demodulator and produces an 48KS/s mono audio
 * stream. Frame contents are sent to the message queue.
 */
class op25_decoder_ff : public gr_block
{
public:

   /**
    * op25_decoder_ff (virtual) destructor.
    */
   virtual ~op25_decoder_ff();

   /**
    * Estimate nof_input_items_reqd for a given nof_output_items.
    */
   virtual void forecast(int nof_output_items, gr_vector_int &nof_input_items_reqd);

   /**
    * Process symbols into frames.
    */
   virtual int general_work(int nof_output_items, gr_vector_int& nof_input_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items);

private:

   /**
    * Expose class to public ctor. Create a new instance of
    * op25_decoder_ff and wrap it in a shared_ptr. This is effectively
    * the public constructor.
    *
    * \param msgq The queue on which to write the decoded messages.
    */
   friend op25_decoder_ff_sptr op25_make_decoder_ff(gr_msg_queue_sptr msgq);

   /**
    * op25_decoder_ff protected constructor.
    *
    * \param msgq A pointer to the msgq.
    */
   op25_decoder_ff(gr_msg_queue_sptr msgq);

   /**
    * Tests whether the received dibit symbol completes a frame sync
    * sequence. Returns true when d completes a frame sync bit string
    * otherwise returns false. When found d_fs contains the frame sync
    * value.
    *
    * \param d The symbol to process.
    */
   bool correlates(dibit d);

   /**
    * Process a received symbol.
    *
    * \param d The symbol to process.
    */
   void receive_symbol(dibit d);

private:
 
   enum { SYNCHRONIZING, IDENTIFYING, READING } d_state;
   float_queue d_audio;
   data_unit_sptr d_data_unit;
   uint32_t d_data_units;
   bit_vector d_frame_hdr;
   frame_sync d_fs;
   imbe_decoder_sptr d_imbe;
   gr_msg_queue_sptr d_msgq;
   uint32_t d_symbol;
   uint32_t d_unrecognized;
};

#endif /* INCLUDED_OP25_DECODER_FF_H */
