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

using std::string;

pdu::pdu(const_bit_queue& frame_body) :
   abstract_data_unit(frame_body)
{
}
pdu::~pdu()
{
}

string
pdu::duid_str() const
{
   return string("PDU");
}

void
pdu::do_correct_errors(bit_vector& frame_body)
{
}

uint16_t
pdu::frame_size_max() const
{
  const size_t HEADER_BLOCK_SIZE = 312;

  // ToDo: decide how do to get header fields when block is not yet complete

  return HEADER_BLOCK_SIZE;
}
