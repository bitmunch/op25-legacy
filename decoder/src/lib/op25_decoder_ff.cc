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
#include <bitset>
#include <gr_io_signature.h>
#include <gr_message.h>
#include <itpp/comm/bch.h>
#include <op25_decoder_ff.h>

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
   const size_t nof_inputs = nof_input_items_reqd.size();
   std::fill(&nof_input_items_reqd[0], &nof_input_items_reqd[nof_inputs], nof_output_items);
}

int  
op25_decoder_ff::general_work(int nof_output_items, gr_vector_int& nof_input_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items)
{
   const float *in = reinterpret_cast<const float*>(input_items[0]);
   for(int i = 0; i < nof_input_items[0]; ++i) {
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
   consume(0, nof_input_items[0]);
   
   for(int i = 0; i < nof_output_items; ++i) {
      float *out = reinterpret_cast<float*>(&output_items[i]);
      std::fill(&out[0], &out[nof_output_items], 0.0); // audio silence - for now
   }
   return nof_output_items;
}

op25_decoder_ff::op25_decoder_ff(gr_msg_queue_sptr msgq) :
   gr_block("decoder_ff", gr_make_io_signature(1, 1, sizeof(float)), gr_make_io_signature(0, 1, sizeof(float))),
   d_state(SYNCHRONIZING),
   d_data_unit(),
   d_data_units(0),
   d_frame_hdr(114),
   d_fs(0),
   d_imbe(imbe_decoder::make_imbe_decoder()),
   d_msgq(msgq),
   d_symbol(0),
   d_unrecognized(0)
{
}

bool
op25_decoder_ff::correlates(dibit d)
{
   d_fs <<= 2;
   d_fs |= d;
   static const frame_sync FS(0x5575f5ff77ffLL);
   const frame_sync diff(FS ^ d_fs);
   return diff.count() < 4;
}

void
op25_decoder_ff::receive_symbol(dibit d)
{
   switch(d_state) {
   case SYNCHRONIZING:
      if(correlates(d)) {
         d_symbol = 23;
         d_state = IDENTIFYING;
         swab(d_fs, 47, -1, d_frame_hdr, 0);
      }
      break;
   case IDENTIFYING:
      ++d_symbol;
      d_frame_hdr[2 * d_symbol] = d & 2;
      d_frame_hdr[2 * d_symbol + 1] = d & 1;
      if(56 == d_symbol) {
         static itpp::BCH bch(63, 16, 11,"6 3 3 1 1 4 1 3 6 7 2 3 5 4 5 3", true);
         itpp::bvec b(63);
         swab(d_frame_hdr,  63, 47, b,  0);
         swab(d_frame_hdr, 112, 71, b, 16);
         swab(d_frame_hdr,  69, 63, b, 57);
         bch.decode(b);
         // ToDo compute/check parity bit?
         swab(b, 57, d_frame_hdr,  69, 63);
         swab(b, 16, d_frame_hdr, 112, 71);
         swab(b,  0, d_frame_hdr,  63, 47);
         d_data_unit = data_unit::make_data_unit(d_frame_hdr);
         if(d_data_unit) {
            d_state = READING;
         } else {
            ++d_unrecognized;
            d_state = SYNCHRONIZING;
         }
      }
      break;
   case READING:
      d_data_unit->extend(d);
      if(d_data_unit->size() == d_data_unit->max_size()) {
         size_t msg_sz = (7 + d_data_unit->size()) >> 3;
         gr_message_sptr msg = gr_make_message(/*type*/0, /*arg1*/++d_data_units, /*arg2*/0, msg_sz);
         uint8_t *msg_data = static_cast<uint8_t*>(msg->msg());
         if((msg_sz = d_data_unit->decode(msg_sz, msg_data, *d_imbe, d_audio))) {
            d_msgq->handle(msg);
         }
         data_unit_sptr null;
         d_data_unit = null;
         d_state = SYNCHRONIZING;
      }
      break;
   }
}
