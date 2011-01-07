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
#include <op25_pcap_source.h>
#include <stdint.h>
#include <stdexcept>
#include <strings.h>

using namespace std;

op25_pcap_source_sptr
op25_make_pcap_source(const char *path)
{
   return op25_pcap_source_sptr(new op25_pcap_source(path));
}

op25_pcap_source::~op25_pcap_source()
{
   if(pcap_) {
      pcap_close(pcap_);
      pcap_ = NULL;
   }
}

int
op25_pcap_source::work(int nof_output_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items)
{
   try {

      char *out = reinterpret_cast<char*>(output_items[0]);
      bzero(out, nof_output_items);
      size_t nof_output_symbols = static_cast<size_t>(nof_output_items);
      while(pcap_ && symbols_.size() < nof_output_symbols) {
         struct pcap_pkthdr hdr;
         const uint8_t *octets = pcap_next(pcap_, &hdr);
         if(octets) {
            const size_t ethernet_sz = 14;
            for(size_t i = ethernet_sz; i < hdr.caplen; ++i) {
               for(int16_t j = 6; j >= 0; j -= 2) {
                  dibit d = (octets[i] >> j) & 0x3;
                  symbols_.push_back(d);
               }
            }
         } else {
            pcap_close(pcap_);
            pcap_ = NULL;
            if(repeat_) {
               char err[PCAP_ERRBUF_SIZE];
               pcap_ = pcap_open_offline(path_.c_str(), err);
            }
         }
      }

      // fill the output buffer and remove used symbols
      size_t nof_symbols = min(symbols_.size(), nof_output_symbols);
      copy(symbols_.begin(), symbols_.begin() + nof_symbols, out);
      symbols_.erase(symbols_.begin(), symbols_.begin() + nof_symbols);
      // ToDo: return -1 if pcap_ == NULL and symbols_.size() == 0
      return nof_symbols;

   } catch(const std::exception& x) {
      cerr << x.what() << endl;
      exit(1);
   } catch(...) {
      cerr << "unhandled exception" << endl;
      exit(2);
   }

}

op25_pcap_source::op25_pcap_source(const char *path) :
   gr_sync_block ("pcap_source", gr_make_io_signature (0, 0, 0), gr_make_io_signature (1, 1, sizeof(uint8_t))),
   path_(path),
   delay_(3),
   repeat_(true),
   pcap_(NULL)
{
   char err[PCAP_ERRBUF_SIZE];
   pcap_ = pcap_open_offline(path_.c_str(), err);
}
