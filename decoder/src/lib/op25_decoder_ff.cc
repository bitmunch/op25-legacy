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

#include <algorithm>
#include <op25_decoder_ff.h>
#include <gr_io_signature.h>
#include <gr_message.h>

op25_decoder_ff_sptr
op25_make_decoder_ff(gr_msg_queue_sptr msgq)
{
   return op25_decoder_ff_sptr(new op25_decoder_ff(msgq));
}

op25_decoder_ff::~op25_decoder_ff()
{
}

void
op25_decoder_ff::forecast(int nof_output_items, gr_vector_int &nof_input_items_reqd)
{
#if 0
   const int nof_symbols_per_LDU = 864;
   const size_t nof_inputs = nof_input_items_reqd.size();
   std::fill(&nof_input_items_reqd[0], &nof_input_items_reqd[nof_inputs], nof_symbols_per_LDU);
#else
   const size_t nof_inputs = nof_input_items_reqd.size();
   std::fill(&nof_input_items_reqd[0], &nof_input_items_reqd[nof_inputs], nof_output_items);
#endif
}

int  
op25_decoder_ff::general_work(int nof_output_items, gr_vector_int& nof_input_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items)
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
   consume_each(nof_input_items[0]);

   // for now audio silence is output
   float *out = reinterpret_cast<float*>(output_items[0]);
   std::fill(&out[0], &out[nof_output_items], 0.0);
   return nof_output_items;
}

op25_decoder_ff::op25_decoder_ff(gr_msg_queue_sptr msgq) :
   gr_block("decoder_ff", gr_make_io_signature(1, 1, sizeof(float)), gr_make_io_signature(1, 1, sizeof(float))),
   d_msgq(msgq),
   d_state(SYNCHRONIZING),
   d_substate(IDENTIFYING),
   d_data_unit(),
   d_data_units(0),
   d_frame_sync(0),
   d_network_ID(0),
   d_symbol(0),
   d_unrecognized(0)
{
}

bool
op25_decoder_ff::correlates(dibit d)
{
   size_t errs = 0;
   const size_t ERR_THRESHOLD = 4;
   const size_t NOF_FS_BITS = 48;
   const uint64_t FS = 0x5575f5ff77ffLL;
   const uint64_t FS_MASK = 0xffffffffffffLL;
   d_frame_sync = (d_frame_sync << 2) | d;
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

bool
op25_decoder_ff::identifies(dibit d)
{
   bool identified = false;
   d_network_ID <<= 2;
   d_network_ID |= d;
   const size_t LAST_NETWORK_ID_SYMBOL = 56;
   if(LAST_NETWORK_ID_SYMBOL == d_symbol) {
      identified = true;
   }
   return identified;
}

void
op25_decoder_ff::receive_symbol(dibit d)
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
      sync_receive_symbol(d);
      ++d_symbol;
      break;
   }
}

void
op25_decoder_ff::sync_receive_symbol(dibit d)
{
   switch(d_substate) {
   case IDENTIFYING:
      if(identifies(d)) {
         d_data_unit = data_unit::make_data_unit(d_frame_sync, d_network_ID);
         if(d_data_unit) {
            d_substate = READING;
         } else {
            ++d_unrecognized;
            d_state = SYNCHRONIZING;
         }
      }
      break;
   case READING:    
      if(d_data_unit->complete(d)) {
         size_t msg_sz = d_data_unit->size();
         gr_message_sptr msg = gr_make_message(/*type*/0, /*arg1*/++d_data_units, /*arg2*/0, msg_sz);
         uint8_t *msg_data = static_cast<uint8_t*>(msg->msg());
         if((msg_sz = d_data_unit->decode(msg_sz, msg_data))) {
            d_msgq->handle(msg);
         }
         data_unit_sptr null;
         d_data_unit = null;
         d_state = SYNCHRONIZING;
      }
      break;
   }
}
