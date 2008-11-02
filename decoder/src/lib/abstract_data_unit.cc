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

#include <abstract_data_unit.h>

#include <algorithm>

using namespace std;

/*
 * Destruct an instance of an abstract_data_unit.
 */
abstract_data_unit::~abstract_data_unit()
{
}

/*
 * Returns the size of this data unit in octets.  For variable-length
 * data units its acceptable to return 0 until the actual length is
 * known.
 */
size_t
abstract_data_unit::size() const
{
   const size_t symbols_per_octet = 4;
   return nof_symbols_reqd() / symbols_per_octet;
}

/*
 * Does the dibit complete the data unit?  If the frame is complete
 * this returns true otherwise returns false.
 */
bool
abstract_data_unit::complete(dibit d)
{
   d_symbols.push_back(d);
   return nof_symbols_reqd() <= d_symbols.size();
}

/*
 * Return the number of symbols in this data_unit. For variable-length
 * data units its acceptable to return 0 until the actual length is
 * known.
 */
size_t
abstract_data_unit::nof_symbols_reqd() const
{
   return 0;
}

/*
 * Decode this data unit.
 */
size_t
abstract_data_unit::decode(size_t &msg_sz, uint8_t *msg)
{
   fill(&msg[0], &msg[msg_sz], 0x0);
   // ToDo: decode the message
   return msg_sz;
}

/*
 * Construct an abstract_data_unit instance.
 */
abstract_data_unit::abstract_data_unit(uint64_t frame_sync, uint64_t network_ID, size_t size_hint) :
   d_frame_sync(frame_sync),
   d_network_ID(network_ID),
   d_symbols(size_hint)
{
   for(size_t i = 0; i < 48; i += 2) {
      dibit d = (frame_sync >> (46 - i)) & 0x3;
      d_symbols.push_back(d);
   }
   for(size_t i = 0; i < 64; i += 2) {
      dibit d = (network_ID >> (62 - i)) & 0x3;
      d_symbols.push_back(d);     
   }
}

/*
 * Return the frame sync value for this data unit.
 */
uint64_t
abstract_data_unit::frame_sync() const
{
   return d_frame_sync;
}

/*
 * Return the network ID for this data unit.
 */
uint64_t
abstract_data_unit::network_ID() const
{
   return d_network_ID;
}

/*
 * Return the index of the last symbol received by this data
 * unit. Note that this is the index after status symbol suppression.
 */
uint64_t
abstract_data_unit::nof_symbols() const
{
   return d_symbols.size();
}

/*
 * Return a pointer to the array of dibit symbols for this data unit.
 */
const dibit*
abstract_data_unit::symbols() const
{
   return d_symbols.data();
}
