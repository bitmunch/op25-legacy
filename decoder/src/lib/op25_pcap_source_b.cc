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
#include <stdexcept>
#include <strings.h>

using namespace std;

op25_pcap_source_b_sptr
op25_make_pcap_source_b(const char *path, float delay, bool repeat)
{
   return op25_pcap_source_b_sptr(new op25_pcap_source_b(path, delay, repeat));
}

op25_pcap_source_b::~op25_pcap_source_b()
{
   if(pcap_) {
      pcap_close(pcap_);
      pcap_ = NULL;
   }
}

int
op25_pcap_source_b::work(int nof_output_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items)
{
   try {
      uint8_t *out = reinterpret_cast<uint8_t*>(output_items[0]);
      const size_t SYMS_REQD = static_cast<size_t>(nof_output_items);
      if(symbols_.size() < SYMS_REQD) {
         read_at_least(SYMS_REQD);
      }
      const size_t SYMS_AVAIL = min(symbols_.size(), SYMS_REQD);
      copy(symbols_.begin(), symbols_.begin() + SYMS_AVAIL, out);
      symbols_.erase(symbols_.begin(), symbols_.begin() + SYMS_AVAIL);
      fill(out + SYMS_AVAIL, out + SYMS_REQD, 0);
      return(0 == SYMS_AVAIL ? -1 : SYMS_REQD);
   } catch(const std::exception& x) {
      cerr << x.what() << endl;
      exit(1);
   } catch(...) {
      cerr << "unhandled exception" << endl;
      exit(2);
   }

}
op25_pcap_source_b::op25_pcap_source_b(const char *path, float delay, bool repeat) :
   gr_sync_block ("pcap_source_b", gr_make_io_signature (0, 0, 0), gr_make_io_signature (1, 1, sizeof(uint8_t))),
   path_(path),
   DELAY_(delay),
   repeat_(repeat),
   pcap_(NULL),
   prev_is_present_(false),
   SYMBOLS_PER_SEC_(4800.0)
{
   char err[PCAP_ERRBUF_SIZE];
   pcap_ = pcap_open_offline(path_.c_str(), err);
}

float
op25_pcap_source_b::ifs(const struct pcap_pkthdr& NOW, const struct pcap_pkthdr& PREV, const size_t HEADER_SZ) const
{
   double t1 = (PREV.ts.tv_usec / 1e6);
   double adj = (NOW.len - HEADER_SZ) * 4.0 / SYMBOLS_PER_SEC_;
   double t2 = (NOW.ts.tv_sec - PREV.ts.tv_sec) + (NOW.ts.tv_usec / 1e6);
   return static_cast<float>(t2 - adj - t1);
}

uint_least32_t
op25_pcap_source_b::read_at_least(const size_t NSYMS_REQD)
{
   size_t n = 0;
   struct pcap_pkthdr hdr;
   const size_t ETHERNET_SZ = 14;
   while(pcap_ && n < NSYMS_REQD) {
      const uint8_t *octets = pcap_next(pcap_, &hdr);
      if(octets) {
         // push inter-frame silence symbols
         const float N = (prev_is_present_ ? ifs(hdr, prev_, ETHERNET_SZ) :  DELAY_);
         const uint_least32_t NSYMS = roundl(N * (1 / SYMBOLS_PER_SEC_));
         for(uint_least32_t i = 0; i < NSYMS; ++i, ++n) {
            symbols_.push_back(0);
         }
         // push symbols from frame payload MSB first
         for(size_t i = ETHERNET_SZ; i < hdr.caplen; ++i, ++n) {
            for(int16_t j = 6; j >= 0; j -= 2) {
               dibit d = (octets[i] >> j) & 0x3;
               symbols_.push_back(d);
            }
         }
         prev_ = hdr;
         prev_is_present_ = true;
      } else {
         pcap_close(pcap_);
         pcap_ = NULL;
         if(repeat_) {
            // re-open the file
            char err[PCAP_ERRBUF_SIZE];
            pcap_ = pcap_open_offline(path_.c_str(), err);
            prev_is_present_ = false;
         }
      }
   }
   return n;
}
