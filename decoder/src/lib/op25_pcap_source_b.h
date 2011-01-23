/* -*- C++ -*- */

/*
 * Copyright 2010-2011 Steve Glass
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

#ifndef INCLUDED_OP25_PCAP_SOURCE_B_H
#define INCLUDED_OP25_PCAP_SOURCE_B_H


#include <boost/shared_ptr.hpp>
#include <gr_sync_block.h>
#define PCAP_DONT_INCLUDE_PCAP_BPF_H
#include <pcap/pcap.h>
#include <stdint.h>
#include <string>
#include <deque>

typedef boost::shared_ptr<class op25_pcap_source_b> op25_pcap_source_b_sptr;

op25_pcap_source_b_sptr op25_make_pcap_source_b(const char *path, float delay, bool repeat);

/**
 * op25_pcap_source_b is a GNU Radio block for reading from a
 * tcpdump-formatted capture file and producing a stream of dibit symbols.
 */
class op25_pcap_source_b : public gr_sync_block
{
public:

   /**
    * op25_pcap_source_b (virtual) destructor.
    */
   virtual ~op25_pcap_source_b();

   /**
    * Process symbols into frames.
    */
   virtual int work(int nof_output_items, gr_vector_const_void_star& input_items, gr_vector_void_star& output_items);

private:

   /**
    * Expose class to public ctor. Create a new instance of
    * op25_pcap_source_b and wrap it in a shared_ptr. This is
    * effectively the public constructor.
    *
    * \param path The path to the tcpdump-formatted input file.
    * \param delay The number of seconds to delay before sending the first frame.
    * \param repeat Loop back to beginning when EOF is encountered.
    */
   friend op25_pcap_source_b_sptr op25_make_pcap_source_b(const char *path, float delay, bool repeat);

   /**
    * op25_pcap_source_b protected constructor.
    *
    * \param path The path to the tcpdump-formatted input file.
    * \param delay The number of seconds to delay before sending the first frame.
    * \param repeat Loop back to beginning when EOF is encountered.
    */
   op25_pcap_source_b(const char *path, float delay, bool repeat);

   /**
    * Compute the interframe space between the frames NOW and PREV and
    * taking care to ignore HEADER_SZ octets of the frame length. The
    * timestamps are presumed to be taken at the end of the frame.
    *
    * \param NOW The pcap_pkthdr for the most recent frame.
    * \param PREV The pcap_pkthdr for the previous frame.
    * \param HEADER_SZ The number of octets in the packet header.
    * \return The interframe space expressed in seconds.
    */
   float ifs(const struct pcap_pkthdr& NOW, const struct pcap_pkthdr& PREV, const size_t HEADER_SZ) const;

   /**
    * Read at least NYSMS_REQD symbols from the tcpdump-formatted
    * file. This method populates the symbols_ queue and may return
    * less than NSYMS_REQD when at end-of-file. When there is no more
    * data it returns zero.
    *
    * \param nsyms_reqd The number of symbols required.
    * \return The actual number of symbols read.
    */
   uint_least32_t read_at_least(size_t nsyms_reqd);

private:

   /**
    * Path to the pcap file.
    */
   std::string path_;

   /**
    * Delay (in seconds) before injecting first frame.
    */
   const float DELAY_;

   /**
    * Repeat the stream when at end?
    */
   bool repeat_;

   /**
    * Handle to the pcap file.
    */
   pcap_t *pcap_;

   /**
    * Details for previous frame.
    */
   struct pcap_pkthdr prev_;

   /**
    * Is prev_ present?
    */
   bool prev_is_present_;

   /**
    * Define dibit type
    */
   typedef uint8_t dibit;

   /**
    * Queue of dibit symbols.
    */
   std::deque<dibit> symbols_;

   /**
    * The number of symbols/s produced by this block.
    */
   const float SYMBOLS_PER_SEC_;

};

#endif /* INCLUDED_OP25_PCAP_SOURCE_B_H */
