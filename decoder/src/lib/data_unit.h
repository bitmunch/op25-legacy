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

#ifndef INCLUDED_DATA_UNIT_H
#define INCLUDED_DATA_UNIT_H

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <stdint.h>

typedef uint8_t dibit;
typedef boost::shared_ptr<class data_unit> data_unit_sptr;

/*
 * A P25 data unit.
 *
 */
class data_unit : public boost::noncopyable
{
public:
   static data_unit_sptr make_data_unit(uint64_t frame_sync, uint64_t network_ID);
   virtual ~data_unit();
   virtual size_t size() const = 0;
   // virtual data_unit_type type() const = 0;
   virtual bool complete(dibit d) = 0;

	/**
    * Decode this data unit. Perform error correction on the received
    * frame and write the corrected frame contents to msg.
    * \param msg_sz The size of the message buffer.
    * \param msg A pointer to where the data unit content will be written.
    * \return The number of octets written to msg.
    */
   virtual size_t decode(size_t msg_sz, uint8_t *msg) = 0;

   // virtual size_t audio(float *samples, size_t max_samples_sz) = 0;
protected:
   data_unit();
};

#endif /* INCLUDED_DATA_UNIT_H */
