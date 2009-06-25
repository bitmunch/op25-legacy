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

#include <boost/scoped_ptr.hpp>
#include <data_unit.h>
#include <data_unit_handler.h>
#include <gr_block.h>
#include <gr_msg_queue.h>
#include <sniffer_du_handler.h>

typedef boost::shared_ptr<class op25_decoder_ff> op25_decoder_ff_sptr;

op25_decoder_ff_sptr op25_make_decoder_ff(gr_msg_queue_sptr msgq);

/**
 * op25_decoder_ff is a GNU Radio block for decoding APCO P25
 * signals. This class expects its input to be a stream of dibit
 * symbols from the demodulator and produces a mono audio
 * stream.
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

   /**
    * Return a pointer to a string naming the device to which packets
    * are forwarded.
    *
    * \return A pointer to a NUL-terminated character string.
    */
   const char *device_name() const;

private:

   /**
    * Expose class to public ctor. Create a new instance of
    * op25_decoder_ff and wrap it in a shared_ptr. This is effectively
    * the public constructor.
    */
   friend op25_decoder_ff_sptr op25_make_decoder_ff(gr_msg_queue_sptr msgq);

   /**
    * op25_decoder_ff protected constructor.
    */
   op25_decoder_ff(gr_msg_queue_sptr msgq);

   /**
    * Tests whether d_frame_header correlates with the APCO P25 frame
    * sync sequence. This method must only be called when the frame
    * header is larger than 48 bits in length (the minimum size for
    * the FS).
    *
    * \return true if the frame header correlates; otherwise false.
    */
   bool correlated();

   /**
    * Tests whether d_frame_header identifies a known data unit and if
    * so sets d_data_unit to point to an appropriate instance. This
    * method must only be called when the frame header is larger than
    * 114 bits in length (the minimum size for a frame containing a
    * NID).
    *
    * \return true if the frame header identifies a known data unit;
    * otherwise false.
    */
   bool identified();

   /**
    * Handle a received symbol.
    *
    * \param d The symbol to process.
    */
   void receive_symbol(dibit d);

private:

   /**
    * When d_state == READING the current data unit, otherwise null.
    */
   data_unit_sptr d_data_unit;

   /**
    * The head of a chain of data_unit_handler instances.
    */
   data_unit_handler_sptr d_data_unit_handler;

   /**
    * A bit_queue used to correlate the FS.
    */
   bit_queue d_frame_hdr;

   /**
    * The message queue used to send snapshots to the UI.
    */
   gr_msg_queue_sptr d_msgq;

   /**
    * Valid states for the decoder state model.
    */
   enum { SYNCHRONIZING, IDENTIFYING, READING } d_state;

   /**
    * The sniffer (TUN/TAP) data unit handler.
    */
   sniffer_du_handler *d_sniffer_du_handler;

};

#endif /* INCLUDED_OP25_DECODER_FF_H */
