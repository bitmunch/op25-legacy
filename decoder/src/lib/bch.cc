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
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with OP25; see the file COPYING. If not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Boston, MA
 * 02110-1301, USA.
 */

#include <bch.h>

/*
 * APCO P25 BCH(64,16,23) encoder.
 */
uint64_t
bch_64_encode(uint16_t val)
{
   static const uint64_t encoding_matrix[] = {
      0x8000cd930bdd3b2aLL,
      0x4000ab5a8e33a6beLL,
      0x2000983e4cc4e874LL,
      0x10004c1f2662743aLL,
      0x0800eb9c98ec0136LL,
      0x0400b85d47ab3bb0LL,
      0x02005c2ea3d59dd8LL,
      0x01002e1751eaceecLL,
      0x0080170ba8f56776LL,
      0x0040c616dfa78890LL,
      0x0020630b6fd3c448LL,
      0x00103185b7e9e224LL,
      0x000818c2dbf4f112LL,
      0x0004c1f2662743a2LL,
      0x0002ad6a38ce9afbLL,
      0x00019b2617ba7657LL
   };

   uint64_t cw = 0LL;
   for(uint8_t i=0; i < 16; ++i) {
      if(val & (1 << (15 - i))) {
         cw ^= encoding_matrix[i];
      }
   }
   return cw;
}

/*
 * APCO P25 BCH(64,16,23) decoder.
 */
bool
bch_64_decode(uint64_t& cw)
{
   // ToDo: implement me

   return true;
}
