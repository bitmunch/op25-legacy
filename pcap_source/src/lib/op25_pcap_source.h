/* -*- C++ -*- */

/*
 * Copyright 2010 Steve Glass
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

#ifndef INCLUDED_OP25_PCAP_SOURCE_H
#define INCLUDED_OP25_PCAP_SOURCE_H


#include <boost/shared_ptr.hpp>
#include <gr_sync_block.h>
#define PCAP_DONT_INCLUDE_PCAP_BPF_H
#include <pcap/pcap.h>
#include <stdint.h>
#include <string>
#include <deque>

typedef boost::shared_ptr<class op25_pcap_source> op25_pcap_source_sptr;

op25_pcap_source_sptr op25_make_pcap_source(const char *path);

/**
 * op25_pcap_source is a GNU Radio block for reading from a
 * tcpdump-formatted capture file and producing a stream of dibit symbols.
 */
class op25_pcap_source : public gr_sync_block
{
public:

   /**
    * op25_pcap_source (virtual) destructor.
    */
   virtual ~op25_pcap_source();

   /**
    * Process symbols into frames.
    */
   virtual int work(int nof_output_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items);

private:

   /**
    * Expose class to public ctor. Create a new instance of
    * op25_pcap_source and wrap it in a shared_ptr. This is
    * effectively the public constructor.
    */
   friend op25_pcap_source_sptr op25_make_pcap_source(const char *path);

   /**
    * op25_pcap_source protected constructor.
    */
   op25_pcap_source(const char *path);

private:

   /**
    * Path to the pcap file.
    */
   std::string path_;

   /**
    * Delay before injecting.
    */
   const uint32_t delay_;

   /**
    * Repeat the stream when at end?
    */
   bool repeat_;

   /**
    * nof symbols produced so far.
    */
   uint32_t nsyms_;

   /**
    * Handle to the pcap file.
    */
   pcap_t *pcap_;

   /**
    * Define dibit type
    */
   typedef uint8_t dibit;

   /**
    * Queue of dibit symbols.
    */
   std::deque<dibit> symbols_;

};

#endif /* INCLUDED_OP25_PCAP_SOURCE_H */
