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

#include <cstdio>
#include <op25_decoder_f.h>
#include <gr_io_signature.h>

/*
 * Create a new instance of op25_decoder_f and wrap it in a
 * shared_ptr. This is effectively the public constructor.
 */
op25_decoder_f_sptr
op25_make_decoder_f(gr_msg_queue_sptr msgq)
{
   return op25_decoder_f_sptr(new op25_decoder_f(msgq));
}

/*
 * Destruct an instance of this class.
 */
op25_decoder_f::~op25_decoder_f()
{
}

/*
 * Take an incoming float value, convert to a symbol and process.
 */
int  
op25_decoder_f::work(int nof_output_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items)
{
   const float *in = reinterpret_cast<const float*>(input_items[0]);
   for(int i = 0; i < nof_output_items; ++i) {
      dibit d;
      if(in[i] < -2.0) {
         d = 3;
      } else if(in[i] <  0.0) {
         d = 2;
      } else if(in[i] <  2.0) {
         d = 0;
      } else {
         d = 1;
      }
      receive_symbol(d);
   }
   return nof_output_items;
}

/*
 * The private constructor.
 */
op25_decoder_f::op25_decoder_f(gr_msg_queue_sptr msgq) :
   gr_sync_block("decoder_f", gr_make_io_signature(1, 1, sizeof(float)), gr_make_io_signature(0, 0, 0)),
   d_msgq(msgq),
   d_state(SYNCHRONIZING),
   d_substate(IDENTIFYING),
   d_frame_sync(0L),
   d_network_ID(0L),
   d_symbol(0),
   d_data_unit(),
   d_data_units(0),
   d_unrecognized(0)
{
}

/*
 * Tests whether the received dibit completes a frame sync
 * sequence. Returns true when d completes a frame sync bit string
 * otherwise returns false. When found d_frame_sync contains the frame
 * sync value.
 */
bool
op25_decoder_f::correlates(dibit d)
{
   size_t errs = 0;
   const size_t ERR_THRESHOLD = 4;
   const size_t NOF_FS_BITS = 48;
   const uint64_t FS = 0x5575f5ff77ffL;
   const uint64_t FS_MASK = 0xffffffffffffL;
   d_frame_sync <<= 2;
   d_frame_sync |= d;
   d_frame_sync &= FS_MASK;
   size_t diff = FS ^ d_frame_sync;
   for(size_t i = 0; i < NOF_FS_BITS; ++i) {
      if(diff & 0x1) {
         ++errs;
      }
      diff >>= 1;
   }
   return errs < ERR_THRESHOLD;
}

/*
 * Tests whether this dibit identifies a known frame type. Returns
 * true when d completes a network ID bit string otherwise returns
 * false. When found d_network_ID contains the network ID value.
 */
bool
op25_decoder_f::identifies(dibit d)
{
   bool identified = false;
   d_network_ID <<= 2;
   d_network_ID |= d;
   const size_t LAST_NETWORK_ID_SYMBOL = 56;
   if(LAST_NETWORK_ID_SYMBOL == d_symbol) {
      // ToDo: BCH (64,16,23) decoding
      identified = true;
   }
   return identified;
}

/*
 * Process a received symbol.
 */
void
op25_decoder_f::receive_symbol(dibit d)
{
   switch(d_state) {
   case SYNCHRONIZING:
      if(correlates(d)) {
         d_symbol = 24;
         d_state = SYNCHRONIZED;
         d_substate = IDENTIFYING;
      }
      break;
   case SYNCHRONIZED:
      const size_t SS_DISTANCE = 36;
      if(d_symbol % SS_DISTANCE) {
         sync_receive_symbol(d);
      } else {
         // d_status_symbols[d_symbol % SS_DISTANCE] = d;
      }
      ++d_symbol;
      break;
   }
}

/*
 * Process a received symbol when synchronized.
 */
void
op25_decoder_f::sync_receive_symbol(dibit d)
{
   switch(d_substate) {
   case IDENTIFYING:
      if(identifies(d)) {
         d_data_unit = data_unit::make_data_unit(d_frame_sync, d_network_ID);
         if(d_data_unit) {
            const char *type_name(typeid(*d_data_unit).name());
            printf("frame_sync: %lx, network_ID: %lx, data_unit: %s\n", d_frame_sync, d_network_ID, type_name);
            d_substate = READING;
         } else {
            ++d_unrecognized;
            d_state = SYNCHRONIZING;
         }
      }
      break;
   case READING:    
      if(d_data_unit->complete(d)) {
         gr_message_sptr msg(d_data_unit->decode());
         if(msg) {
            // ToDo: prefix frame with status symbols + other header stuff
            d_msgq->insert_tail(msg);
         }
         ++d_data_units;
         data_unit_sptr null;
         d_data_unit = null;
         d_state = SYNCHRONIZING;
      }
      break;
   }
}
