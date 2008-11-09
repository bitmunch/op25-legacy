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

#include <bch.h>
#include <data_unit.h>
#include <hdu.h>
#include <ldu1.h>
#include <ldu2.h>
#include <pdu.h>
#include <tdu.h>

data_unit_sptr
data_unit::make_data_unit(uint64_t fs, uint64_t nid)
{
   data_unit_sptr d;
   if(bch_64_decode(nid)) {
      uint8_t duid = (nid >> 48) & 0xf;
      switch(duid) {
      case 0x0:
         d = data_unit_sptr(new hdu(fs, nid));
         break;
      case 0x3:
         d = data_unit_sptr(new tdu(fs, nid, false));
         break;
      case 0x5:
         d = data_unit_sptr(new ldu1(fs, nid));
         break;
      case 0xa:
         d = data_unit_sptr(new ldu2(fs, nid));
         break;
      case 0x9: // VSELP "voice pdu"
      case 0xc:
         d = data_unit_sptr(new pdu(fs, nid));
         break;
      case 0xf:
         d = data_unit_sptr(new tdu(fs, nid, true));
         break;
      };
   }
   return d;
}

data_unit::~data_unit()
{
}

data_unit::data_unit()
{
}
