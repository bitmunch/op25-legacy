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
#include <stdexcept>

using namespace std;

abstract_data_unit::~abstract_data_unit()
{
}

uint16_t
abstract_data_unit::size() const
{
   return d_frame_body.size();
}

void
abstract_data_unit::extend(dibit d)
{
   // ToDo if(too big) throw range_error();
   d_frame_body.push_back(d & 0x2);
   d_frame_body.push_back(d & 0x1);
}

size_t
abstract_data_unit::decode(size_t msg_sz, uint8_t *msg, imbe_decoder& imbe, float_queue& audio)
{
   correct_errors(d_frame_body);
   size_t nof_octets = decode_body(d_frame_body, msg_sz, msg);
   decode_audio(d_frame_body, imbe, audio);
   return nof_octets;
}

abstract_data_unit::abstract_data_unit(const_bit_vector& frame_body) :
   d_frame_body(frame_body)
{
}

void
abstract_data_unit::correct_errors(bit_vector& frame_body)
{
}

size_t
abstract_data_unit::decode_body(const_bit_vector& frame_body, size_t msg_sz, uint8_t *msg)
{
   // ToDo: default implementation - marshall bits from frame into msg

   return 0;
}

size_t
abstract_data_unit::decode_audio(const_bit_vector& frame_body, imbe_decoder& imbe, float_queue& audio)
{
   return 0;
}
