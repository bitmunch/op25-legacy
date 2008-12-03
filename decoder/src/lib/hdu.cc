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

#include <hdu.h>
#include <itpp/comm/egolay.h>
#include <itpp/comm/reedsolomon.h>

hdu::hdu(const_bit_vector& frame_body) :
   abstract_data_unit(frame_body)
{
}

hdu::~hdu()
{
}

uint16_t
hdu::max_size() const
{
   return 792;
}

void
hdu::correct_errors(bit_vector& frame)
{
   apply_golay_correction(frame);
   apply_rs_correction(frame);
}

void
hdu::apply_golay_correction(bit_vector& frame)
{
   static itpp::Extended_Golay golay;

}

void
hdu::apply_rs_correction(bit_vector& frame)
{
   // static itpp::Reed_Solomon rs(63, 47, 17, true);
}
