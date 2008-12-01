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

#ifndef INCLUDED_PDU_H
#define INCLUDED_PDU_H

#include <abstract_data_unit.h>

/**
 * P25 packet data unit (PDU).
 */
class pdu : public abstract_data_unit
{
public:

   /**
    * P25 packet data unit (PDU) constructor.
    *
    * \param fs The frame synchronization header.
    * \param nid The network ID.
    */
   pdu(frame_sync& fs, network_id& nid);


   /**
    * pdu (virtual) destructor.
    */
   virtual ~pdu();

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
   virtual size_t decode_body(const_bit_vector& frame_body, size_t msg_sz, uint8_t *msg, imbe_decoder& imbe, float_queue& audio);

};

#endif /* INCLUDED_PDU_H */
