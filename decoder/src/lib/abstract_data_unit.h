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

typedef std::vector<dibit> dibit_vector;

/*
 * Abstract P25 data unit, provides common behaviours for subclasses.
 */
class abstract_data_unit : public data_unit
{
public:
   virtual ~abstract_data_unit();
   virtual size_t size() const;
   virtual bool complete(dibit d);
   virtual size_t nof_symbols_reqd() const = 0;
   virtual size_t decode(size_t &msg_sz, uint8_t *msg);
protected:
   abstract_data_unit(uint64_t frame_sync, uint64_t network_ID, size_t size_hint = 0);
   uint64_t frame_sync() const;
   uint64_t network_ID() const;
   uint64_t nof_symbols() const;
   const dibit *symbols() const;
private:
   uint64_t d_frame_sync;
   uint64_t d_network_ID;
   dibit_vector d_symbols;
};

#endif /* INCLUDED_ABSTRACT_DATA_UNIT_H */
