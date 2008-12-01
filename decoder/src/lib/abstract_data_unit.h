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
#include <vector>

typedef std::vector<bool> bit_vector;
typedef const std::vector<bool> const_bit_vector;

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
    * Returns the actual size of this data_unit in bits.
    *
    * \return The size (in dibits) of this data_unit.
    */
   virtual uint16_t size() const;

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
    * Decode, error correct and write the decoded frame contents to
    * msg. If the frame contains compressed audio then the audio
    * should be decoded using the supplied imbe_decoder and write it
    * to audio.
    *
    * \param msg_sz The size of the message buffer.
    * \param msg A pointer to where the data unit content will be written.
    * \param imbe The imbe_decoder to use to generate the audio.
    * \param audio A deque<float> to which the audio (if any) is appended.
    * \return The number of octets written to msg.
    */
   virtual size_t decode(size_t msg_sz, uint8_t *msg, imbe_decoder& imbe, float_queue& audio);

protected:

   /**
    * abstract_data_unit constructor.
    *
    * \param fs The frame synchronization header.
    * \param nid The network ID.
    */
   abstract_data_unit(frame_sync& fs, network_id& nid);

   /**
    * Decode frame_body, apply error correction and write the decoded
    * frame contents to msg. If the frame contains compressed audio
    * the audio should be decoded using the supplied imbe_decoder and
    * written to audio.
    *
    * \param frame_body The bit vector to decode.
    * \param msg_sz The size of the message buffer.
    * \param msg A pointer to where the data unit content will be written.
    * \param imbe The imbe_decoder to use to generate the audio.
    * \param audio A deque<float> to which the audio (if any) is appended.
    * \return The number of octets written to msg.
    */
   virtual size_t decode_body(const_bit_vector& frame_body, size_t msg_sz, uint8_t *msg, imbe_decoder& imbe, float_queue& audio) = 0;

private:

   /**
    * The frame sync header.
    */
   frame_sync d_fs;

   /**
    * The network ID header.
    */
   network_id d_nid;

   /**
    * A bit vector containing the frame body.
    */
   bit_vector d_frame_body;

};

#endif /* INCLUDED_ABSTRACT_DATA_UNIT_H */
