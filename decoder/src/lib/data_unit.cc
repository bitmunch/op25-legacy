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

#include <data_unit.h>
#include <header.h>
#include <ldu1.h>
#include <ldu2.h>
#include <packet.h>
#include <terminator.h>

data_unit_sptr
data_unit::make_data_unit(uint64_t frame_sync, uint64_t network_ID)
{
   data_unit_sptr d;
   uint8_t type = (network_ID >> 48) & 0xf;
   switch(type) {
   case 0x0:
      d = data_unit_sptr(new header(frame_sync, network_ID));
      break;
   case 0x3:
      d = data_unit_sptr(new terminator(frame_sync, network_ID, false));
      break;
   case 0x5:
      d = data_unit_sptr(new ldu1(frame_sync, network_ID));
      break;
   case 0xa:
      d = data_unit_sptr(new ldu2(frame_sync, network_ID));
      break;
   case 0x9: // VSELP "voice packet"
   case 0xc:
      d = data_unit_sptr(new packet(frame_sync, network_ID));
      break;
   case 0xf:
      d = data_unit_sptr(new terminator(frame_sync, network_ID, true));
      break;
   };
   return d;
}

data_unit::~data_unit()
{
}

data_unit::data_unit()
{
}
