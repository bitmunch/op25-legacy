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
#include <iostream>
#include <net/if.h>
#include <linux/if_tun.h>
#include <op25_decoder_ff.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

op25_decoder_ff_sptr
op25_make_decoder_ff()
{
   return op25_decoder_ff_sptr(new op25_decoder_ff);
}

op25_decoder_ff::~op25_decoder_ff()
{
   if(-1 != d_tap) {
      close(d_tap);
   }
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
   try {

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

   } catch(const std::exception& x) {
      cerr << x.what() << endl;
      exit(1);
   } catch(...) {
      cerr << "unhandled exception" << endl;
      exit(2);
   }
}

const char*
op25_decoder_ff::device_name() const
{
   return d_tap_device.c_str();
}

op25_decoder_ff::op25_decoder_ff() :
   gr_block("decoder_ff", gr_make_io_signature(1, 1, sizeof(float)), gr_make_io_signature(0, 1, sizeof(float))),
   d_state(SYNCHRONIZING),
   d_bch(63, 16, 11,"6 3 3 1 1 4 1 3 6 7 2 3 5 4 5 3", true),
   d_data_unit(),
   d_data_units(0),
   d_frame_hdr(114),
   d_fs(0),
   d_imbe(imbe_decoder::make_imbe_decoder()),
   d_symbol(0),
   d_tap(-1),
   d_tap_device("TUN/TAP not available"),
   d_unrecognized(0)
{
   d_tap = open("/dev/net/tun", O_WRONLY);
   if(-1 != d_tap) {
      struct ifreq ifr;
      memset(&ifr, 0, sizeof(ifr));
      ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
      strncpy(ifr.ifr_name, "p25-%d", IFNAMSIZ);
      if(0 == ioctl(d_tap, TUNSETIFF, &ifr)) {
         int s = socket(AF_INET, SOCK_DGRAM, 0);
         if(0 <= s) {
            if(0 == ioctl(s, SIOCGIFFLAGS, &ifr)) {
               ifr.ifr_flags = IFF_UP;
               if(0 == ioctl(s, SIOCSIFFLAGS, &ifr)) {
                  d_tap_device = std::string(ifr.ifr_name);
               } else {
                  perror("ioctl(d_tap, SIOCSIFFLAGS, &ifr)");
                  close(d_tap);
                  d_tap = -1;
               }
            } else {
               perror("ioctl(d_tap, SIOCGIFFLAGS, &ifr)");
               close(d_tap);
               d_tap = -1;
            }
            close(s);
         } else {
            perror("socket(AF_INET, SOCK_DGRAM, 0)");
            close(d_tap);
            d_tap = -1;           
         }
      } else {
         perror("ioctl(d_tap, TUNSETIFF, &ifr)");
         close(d_tap);
         d_tap = -1;
      }
   }
}

bool
op25_decoder_ff::correlates(dibit d)
{
   size_t errs = 0;
   const size_t ERR_THRESHOLD = 4;
   const size_t NOF_FS_BITS = 48;
   const frame_sync FS = 0x5575f5ff77ffLL;
   const frame_sync FS_MASK = 0xffffffffffffLL;
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

         itpp::bvec b(63);
         swab(d_frame_hdr,  63, 47, b,  0);
         swab(d_frame_hdr, 112, 71, b, 16);
         swab(d_frame_hdr,  69, 63, b, 57);
         b = d_bch.decode(b);
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
      if(d_data_unit->is_complete()) {
         const size_t msg_hdr_sz = 14;
         const size_t msg_sz = d_data_unit->data_size();
         const size_t total_msg_sz = msg_hdr_sz + msg_sz;
         uint8_t msg_data[total_msg_sz];
         if(d_data_unit->decode(msg_sz, msg_data + msg_hdr_sz, *d_imbe, d_audio) && (-1 != d_tap)) {
            memset(&msg_data[0], 0xff, 6);
            memset(&msg_data[6], 0x00, 6);
            memset(&msg_data[12], 0xff, 2);
            write(d_tap, msg_data, total_msg_sz);
         }
         data_unit_sptr null;
         d_data_unit = null;
         d_state = SYNCHRONIZING;
      }
      break;
   }
}
