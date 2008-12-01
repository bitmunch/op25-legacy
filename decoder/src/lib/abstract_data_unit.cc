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
   return d_fs.size() + d_nid.size() + d_frame_body.size();
}

void
abstract_data_unit::extend(dibit d)
{
   d_frame_body.push_back(d & 0x2);
   d_frame_body.push_back(d & 0x1);

   // ToDo if(too big) throw range_error();
}

size_t
abstract_data_unit::decode(size_t msg_sz, uint8_t *msg, imbe_decoder& imbe, float_queue& audio)
{
   // ToDo: write d_fs and d_nid into start of msg!

   return decode_body(d_frame_body, msg_sz, msg, imbe, audio);
}

abstract_data_unit::abstract_data_unit(frame_sync& fs, network_id& nid) :
   d_fs(fs),
   d_nid(nid)
{
}
