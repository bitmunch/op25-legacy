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

#ifndef INCLUDED_TDU_H
#define INCLUDED_TDU_H

#include <abstract_data_unit.h>

/*
 * P25 terminator data unit (TDU).
 */
class tdu : public abstract_data_unit
{
public:

   /**
    * tdu constructor.
    */
   tdu(uint64_t frame_sync, uint64_t network_ID, bool has_link_control);

   /**
    * tdu constructor.
    */
   virtual ~tdu();

protected:

   /**
    * Returns the number of symbols required by this data_unit.  For
    * variable-length data this may return 0 if the actual length is
    * not yet known.
    * \return The size in of this data_unit.
    */
   virtual size_t nof_symbols_reqd() const;

   /**
    * Correct any errors found in the dibit stream.
    * \param symbols The dibit stream to correct.
    */
   virtual void correct_errors(dibit_vector& symbols);

   /**
    * Decode this data unit. Perform error correction on the received
    * frame and write the corrected frame contents to msg.
    * \param msg_sz The size of the message buffer.
    * \param msg A pointer to where the data unit content will be written.
    * \param symbols The dibit_vector which is to be decode_symbolsd.
    * \return The number of octets written to msg.
    */
   virtual size_t decode_symbols(size_t msg_sz, uint8_t *msg, const_dibit_vector& symbols);

private:
   bool d_has_link_control;

};

#endif /* INCLUDED_TDU_H */
