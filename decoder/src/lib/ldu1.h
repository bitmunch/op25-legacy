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

#ifndef INCLUDED_LDU1_H
#define INCLUDED_LDU1_H

#include <abstract_data_unit.h>

/**
 * P25 Logical Data Unit 1 (compressed IBME voice).
 */
class ldu1 : public abstract_data_unit
{
public:

   /**
    * ldu1 constuctor
    *
    * \param frame_body A const_bit_vector representing the frame body.
    */
   ldu1(const_bit_vector& frame_body);

   /**
    * ldu1 (virtual) destuctor
    */
   virtual ~ldu1();

   /**
    * Returns the expected size of this data_unit in bits. For
    * variable-length data this should return UINT16_MAX until the
    * actual length of this frame is known.
    *
    * \return The expected size (in bits) of this data_unit.
    */
   virtual uint16_t max_size() const;

protected:

   /**
    * Applies error correction code to the specified bit_vector. Not
    * that this function removes the PN sequences from the source
    * frame.
    *
    * \param frame_body The bit vector to decode.
    * \return 
    */
   virtual void correct_errors(bit_vector& frame_body);

   /**
    * Decode compressed audio using the supplied imbe_decoder and
    * writes output to audio.
    *
    * \param frame_body The const_bit_vector to decode.
    * \param imbe The imbe_decoder to use to generate the audio.
    * \param audio A deque<float> to which the audio (if any) is appended.
    * \return The number of samples written to audio.
    */
   virtual size_t decode_audio(const_bit_vector& frame_body, imbe_decoder& imbe, float_queue& audio);

private:

   /**
    * Compute the PN sequence for this voice
    */
   void remove_pn_sequence(compressed_voice_sample u0);

};

#endif /* INCLUDED_LDU1_H */
