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

#include <itpp/base/vec.h>
#include <itpp/comm/egolay.h>
#include <itpp/comm/reedsolomon.h>
#include <swab.h>
#include <tdu.h>

tdu::tdu(const_bit_vector& frame_body, bool has_link_control) :
   abstract_data_unit(frame_body),
   d_has_link_control(has_link_control)
{
}

tdu::~tdu()
{
}

uint16_t
tdu::max_size() const
{
   return d_has_link_control ? 432 : 144;
}

void
tdu::correct_errors(bit_vector& frame)
{
   if(d_has_link_control) {
      apply_golay_correction(frame);
      apply_rs_correction(frame);
   }
}

void
tdu::apply_golay_correction(bit_vector& frame)
{
   static itpp::Extended_Golay golay;
}

void
tdu::apply_rs_correction(bit_vector& frame)
{
   static itpp::Reed_Solomon rs(63, 17, true);

   itpp::bvec b(70);
   swab(frame, 114, 122, b,  0);
   swab(frame, 122, 126, b,  8);
   swab(frame, 138, 142, b, 12);
   swab(frame, 144, 152, b, 16);
   swab(frame, 164, 176, b, 24);
   swab(frame, 188, 200, b, 36);
   swab(frame, 212, 213, b, 48);
   swab(frame, 216, 226, b, 50);
   swab(frame, 238, 250, b, 60);
   itpp::bvec bd(rs.decode(b));
}

#if 0

void
tdu::encode()
{
   itpp::bvec be(rs.encode(bd));
   swab(be, 60, frame, 238, 250);
   swab(be, 50, frame, 216, 226);
   swab(be, 48, frame, 212, 213);
   swab(be, 36, frame, 188, 200);
   swab(be, 24, frame, 164, 176);
   swab(be, 16, frame, 144, 152);
   swab(be, 12, frame, 138, 142);
   swab(be,  8, frame, 122, 126);
   swab(be,  0, frame, 114, 122);
}

#endif
