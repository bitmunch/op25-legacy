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
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with OP25; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Boston, MA
 * 02110-1301, USA.
 */
#include <pdu.h>

pdu::pdu(uint64_t frame_sync, uint64_t network_ID) :
   abstract_data_unit(frame_sync, network_ID)
{
}

pdu::~pdu()
{
}

size_t
pdu::nof_symbols_reqd() const
{
   size_t reqd = 0;
#if 1
   const size_t LAST_HEADER_BLOCK_SYMBOL = 156;
   reqd = LAST_HEADER_BLOCK_SYMBOL;
#else
   const size_t LAST_HEADER_BLOCK_SYMBOL = 156;
   // ToDo pass nof_symbols_read as input param
   if(LAST_HEADER_BLOCK_SYMBOL < nof_symbols_read) {
      // ToDo: 1/2 rate trellis decoding of header
      reqd = LAST_HEADER_BLOCK_SYMBOL; // force c
      // const size_t SYMBOLS_PER_BLOCK = 98;
      // ToDo: reqd = LAST_HEADER_BLOCK_SYMBOL + header.blocks_to_follow * SYMBOLS_PER_BLOCK;
   }
#endif
   return reqd;
}

void
pdu::correct_errors(dibit_vector& symbols)
{
}

size_t
pdu::decode_symbols(size_t msg_sz, uint8_t *msg, const_dibit_vector& symbols)
{
   return 0;
}
