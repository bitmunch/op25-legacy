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

#ifndef INCLUDED_ABSTRACT_DATA_UNIT_H
#define INCLUDED_ABSTRACT_DATA_UNIT_H

#include <data_unit.h>
#include <string>
#include <vector>

/**
 * Abstract P25 data unit.
 */
class abstract_data_unit : public data_unit
{

public:

   /**
    * abstract data_unit virtual destructor.
    */
   virtual ~abstract_data_unit();

   /**
    * Returns the size (in octets) of the error-corrected and
    * de-interleaved data_unit.
    *
    * \return The expected size (in bits) of this data_unit.
    */
   virtual uint16_t data_size() const;

   /**
    * Extends this data_unit with the specified dibit. If this
    * data_unit is already complete a range_error is thrown.
    *
    * \param d The dibit to extend the frame with.
    * \throws range_error When the frame already is at its maximum size.
    * \return true when the frame is complete otherwise false.
    */
   virtual void extend(dibit d);

   /**
    * Decode, apply error correction and write the decoded frame
    * contents to msg. For voice frames decoding causes the compressed
    * audio to be decoded using the supplied imbe_decoder.
    *
    * \param msg_sz The size of the message buffer.
    * \param msg A pointer to the message buffer.
    * \param imbe The imbe_decoder to use to generate the audio.
    * \param audio A float_queue to which the audio (if any) is written.
    * \return The number of octets written to msg.
    */
   virtual size_t decode(size_t msg_sz, uint8_t *msg, imbe_decoder& imbe, float_queue& audio);

   /**
    * Tests whether this data unit has enough data to begin decoding.
    *
    * \return true when this data_unit is complete; otherwise returns
    * false.
    */
   virtual bool is_complete() const;

   /**
    * Return a snapshot of the key fields from this frame in a manner
    * suitable for display by the UI. The string is encoded as a
    * pickled Python dictionary.
    * 
    * \return A string containing the fields to display.
    */
   virtual std::string snapshot() const;

protected:

   /**
    * abstract_data_unit constructor.
    *
    * \param frame_body A const_bit_queue representing the frame body.
    */
   abstract_data_unit(const_bit_queue& frame_body);

   /**
    * Applies error correction code to the specified bit_vector.
    *
    * \param frame_body The bit vector to decode.
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

   /**
    * Decode frame_body and write the decoded frame contents to msg.
    *
    * \param frame_body The bit vector to decode.
    * \param msg_sz The size of the message buffer.
    * \param msg A pointer to where the data unit content will be written.
    * \return The number of octets written to msg.
    */
   virtual size_t decode_body(const_bit_vector& frame_body, size_t msg_sz, uint8_t *msg);

   /**
    * Returns a string describing the Data Unit ID (DUID).
    *
    * \return A string identifying the DUID.
    */
   virtual std::string duid_str() const = 0;

   /**
    * Returns the expected size (in bits) of this data_unit. For
    * variable-length data this should return UINT16_MAX until the
    * actual length of this frame is known.
    *
    * \return The expected size (in bits) of this data_unit when encoded.
    */
   virtual uint16_t frame_size_max() const = 0;

   /**
    * Returns the current size (in bits) of this data_unit.
    *
    * \return The current size (in bits) of this data_unit.
    */
   virtual uint16_t frame_size() const;

private:

   /**
    * A bit vector containing the frame body.
    */
   bit_vector d_frame_body;

};

#endif /* INCLUDED_ABSTRACT_DATA_UNIT_H */
