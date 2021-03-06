/* -*- C++ -*- */

/*
 * Copyright 2008-2011 Steve Glass
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
#include <gr_io_signature.h>
#include <iostream>
#include <itpp/comm/bch.h>
#include <logfile_du_handler.h>
#include <offline_imbe_decoder.h>
#include <op25_decoder_ff.h>
#include <snapshot_du_handler.h>
#include <p25cai_du_handler.h>
#include <voice_du_handler.h>
#include <op25_yank.h>

using namespace std;

op25_decoder_ff_sptr
op25_make_decoder_ff()
{
   return op25_decoder_ff_sptr(new op25_decoder_ff);
}

op25_decoder_ff::~op25_decoder_ff()
{
}

gr_msg_queue_sptr
op25_decoder_ff::get_msgq() const
{
   return d_snapshot_du_handler->get_msgq();
}

void

op25_decoder_ff::set_msgq(gr_msg_queue_sptr msgq)
{
   d_snapshot_du_handler->set_msgq(msgq);
}

void
op25_decoder_ff::forecast(int nof_output_items, gr_vector_int &nof_input_items_reqd)
{
   /* This block consumes 4800 symbols/s and produces 8000
    * samples/s. That's a work rate of 3/5 or 0.6. If no audio output
    * is available we'll produce silence.
    */
   const size_t nof_inputs = nof_input_items_reqd.size();
   const int nof_samples_reqd = .6 * nof_output_items;
   fill(&nof_input_items_reqd[0], &nof_input_items_reqd[nof_inputs], nof_samples_reqd);
}

int
op25_decoder_ff::general_work(int nof_output_items, gr_vector_int& nof_input_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items)
{
   try {

      // process input
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
      consume_each(nof_input_items[0]);

      // produce audio
      audio_samples *samples = d_imbe->audio();
      float *out = reinterpret_cast<float*>(output_items[0]);
      const int n = min(static_cast<int>(samples->size()), nof_output_items);
      if(0 < n) {
         copy(samples->begin(), samples->begin() + n, out);
         samples->erase(samples->begin(), samples->begin() + n);
      }
      if(n < nof_output_items) {
         fill(out + n, out + nof_output_items, 0.0);
      }
      return nof_output_items;

   } catch(const std::exception& x) {
      cerr << x.what() << endl;
      exit(1);
   } catch(...) {
      cerr << "unhandled exception" << endl;
      exit(2);   }
}

const char*
op25_decoder_ff::destination() const
{
   return d_p25cai_du_handler->destination();
}

op25_decoder_ff::op25_decoder_ff() :
   gr_block("decoder_ff", gr_make_io_signature(1, 1, sizeof(float)), gr_make_io_signature(0, 1, sizeof(float))),
   d_data_unit(),
   d_data_unit_handler(),
   d_frame_hdr(),
   d_imbe(imbe_decoder::make()),
   d_state(SYNCHRONIZING),
   d_p25cai_du_handler(NULL)
{
   d_p25cai_du_handler =  new p25cai_du_handler(d_data_unit_handler, "224.0.0.1", 23456);
   d_data_unit_handler = data_unit_handler_sptr(d_p25cai_du_handler);
   d_snapshot_du_handler = new snapshot_du_handler(d_data_unit_handler);
   d_data_unit_handler = data_unit_handler_sptr(d_snapshot_du_handler);
   d_data_unit_handler = data_unit_handler_sptr(new voice_du_handler(d_data_unit_handler, d_imbe));
}

bool
op25_decoder_ff::correlated()
{
   static const bool FS[] = {
      0, 1, 0, 1, 0, 1, 0, 1,
      0, 1, 1, 1, 0, 1, 0, 1, 
      1, 1, 1, 1, 0, 1, 0, 1,
      1, 1, 1, 1, 1, 1, 1, 1, 
      0, 1, 1, 1, 0, 1, 1, 1,
      1, 1, 1, 1, 1, 1, 1, 1
   };
   static const size_t FS_SZ = sizeof(FS)/sizeof(FS[0]);

   uint8_t errs = 0;
   for(size_t i = 0; i < FS_SZ; ++i) {
      if(d_frame_hdr[i] ^ FS[i]) {
         ++errs;
      }
   }
   return (errs <= 4);
}

data_unit_sptr
op25_decoder_ff::identified()
{
   static const size_t NID[] = {
      63,  62,  61,  60,  59,  58,  57,  56,
      55,  54,  53,  52,  51,  50,  49,  48,
      112, 111, 110, 109, 108, 107, 106, 105,
      104, 103, 102, 101, 100, 99,  98,  97,
      96,  95,  94,  93,  92,  91,  90,  89,
      88,  87,  86,  85,  84,  83,  82,  81,
      80,  79,  78,  77,  76,  75,  74,  73,
      72,  69,  68,  67,  66,  65,  64,
   };
   size_t NID_SZ = sizeof(NID) / sizeof(NID[0]);

   itpp::bvec b(63), zeroes(16);
   itpp::BCH bch(63, 16, 11, "6 3 3 1 1 4 1 3 6 7 2 3 5 4 5 3", true);
   yank(d_frame_hdr,  NID, NID_SZ, b, 0);
   b = bch.decode(b);
   if(b != zeroes) {
      b = bch.encode(b);
      yank_back(b, 0, d_frame_hdr, NID, NID_SZ);
      d_data_unit = data_unit::make_data_unit(d_frame_hdr);
   } else {
      data_unit_sptr null;
      d_data_unit = null;
   }
   return d_data_unit;
}

void
op25_decoder_ff::receive_symbol(dibit d)
{
   d_frame_hdr.push_back(d & 0x2);
   d_frame_hdr.push_back(d & 0x1);
   const size_t frame_hdr_sz = d_frame_hdr.size();

   switch(d_state) {
   case SYNCHRONIZING:
      if(48 <= frame_hdr_sz) {
         d_frame_hdr.erase(d_frame_hdr.begin(), d_frame_hdr.begin() + (frame_hdr_sz - 48));
         if(correlated()) {
            d_state = IDENTIFYING;
         }
      }
      break;
   case IDENTIFYING:
      if(114 == frame_hdr_sz) {
         if(identified()) {
            d_state = READING;
         } else {
            d_state = SYNCHRONIZING;
         }
      }
      break;
   case READING:
      d_data_unit->extend(d);
      if(d_data_unit->is_complete()) {
         d_data_unit->correct_errors();
         d_data_unit_handler->handle(d_data_unit);
         data_unit_sptr null;
         d_data_unit = null;
         d_state = SYNCHRONIZING;
      }
      break;
   }
}
