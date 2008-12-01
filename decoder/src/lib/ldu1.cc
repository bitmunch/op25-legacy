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

#include <ldu1.h>

ldu1::ldu1(frame_sync& fs, network_id& nid) :
   abstract_data_unit(fs, nid)
{
}

ldu1::~ldu1()
{
}

uint16_t
ldu1::max_size() const
{
   return 1728;
}

size_t
ldu1::decode_body(const_bit_vector& frame_body, size_t msg_sz, uint8_t *msg, imbe_decoder& imbe, float_queue& audio)
{
   return max_size() / 8;
}
