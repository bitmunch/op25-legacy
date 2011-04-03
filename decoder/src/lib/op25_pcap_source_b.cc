/* -*- C++ -*- */

/*
 * Copyright 2010,2011 Steve Glass
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

#define __STDC_CONSTANT_MACROS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>
#include <gr_io_signature.h>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <op25_pcap_source_b.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdexcept>
#include <strings.h>

#define PCAP_DONT_INCLUDE_PCAP_BPF_H
#include <pcap/pcap.h>

using namespace std;

op25_pcap_source_b_sptr
op25_make_pcap_source_b(const char *path, float delay)
{
   return op25_pcap_source_b_sptr(new op25_pcap_source_b(path, delay));
}

op25_pcap_source_b::~op25_pcap_source_b()
{
}

int
op25_pcap_source_b::work(int nof_output_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items)
{
   try {
      uint8_t *out = reinterpret_cast<uint8_t*>(output_items[0]);
      const size_t SYMS_AVAIL = symbols_.size();
      const size_t SYMS_REQD = static_cast<size_t>(nof_output_items);
      for(size_t i = 0; i < SYMS_REQD; ++i) {
         out[i] = symbols_[loc_++];
         loc_ %= SYMS_AVAIL;
      }
      return SYMS_REQD;
   } catch(const std::exception& x) {
      cerr << x.what() << endl;
      exit(EXIT_FAILURE);
   } catch(...) {
      cerr << "unhandled exception" << endl;
      exit(EXIT_FAILURE);
   }

}

op25_pcap_source_b::op25_pcap_source_b(const char *path, float delay) :
   gr_sync_block ("pcap_source_b", gr_make_io_signature (0, 0, 0), gr_make_io_signature (1, 1, sizeof(uint8_t))),
   loc_(0),
   SYMBOLS_PER_SEC_(4800.0),
   symbols_(delay * SYMBOLS_PER_SEC_, 0)
{
   pcap_t *pcap;
   char err[PCAP_ERRBUF_SIZE];
   pcap = pcap_open_offline(path, err);
   if(pcap) {
      struct pcap_pkthdr hdr;
      for(const uint8_t *octets; octets = pcap_next(pcap, &hdr);) {
         const size_t ETHERNET_SZ = 14;
         const size_t IP_SZ = 20;
         const size_t UDP_SZ = 8;
         const size_t P25CAI_OFS = ETHERNET_SZ + IP_SZ + UDP_SZ;
         if(P25CAI_OFS < hdr.caplen) {
            const size_t FRAME_SZ = hdr.caplen - P25CAI_OFS;
            // push some zero symbols to separate frames
            const size_t SILENCE_SYMS = 48;
            symbols_.resize(symbols_.size() + SILENCE_SYMS, 0);
            // push symbols from frame payload MSB first
            symbols_.reserve(symbols_.capacity() + ((hdr.caplen - P25CAI_OFS) * 4));
            for(size_t i = 0; i < FRAME_SZ; ++i) {
               for(int16_t j = 6; j >= 0; j -= 2) {
                  dibit d = (octets[P25CAI_OFS + i] >> j) & 0x3;
                  symbols_.push_back(d);
               }
            }
         }
      }
      pcap_close(pcap);
   } else {
      cerr << "error: failed to open " << path;
      cerr << " (" << err << ")" << endl;
      exit(EXIT_FAILURE);
   }
}
