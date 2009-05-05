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

#include <abstract_data_unit.h>

#include <algorithm>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <utility>

using namespace std;

abstract_data_unit::~abstract_data_unit()
{
}

uint16_t
abstract_data_unit::data_size() const
{
   return (7 + frame_size_max()) >> 3;
}

void
abstract_data_unit::extend(dibit d)
{
   if(frame_size_max() <= frame_size()) {
      ostringstream msg;
      msg << "cannot extend frame " << endl;
      msg << "(size now: " << frame_size() << ", expected size: " << frame_size_max() << ")" << endl;
      msg << "func: " << __PRETTY_FUNCTION__ << endl;
      msg << "file: " << __FILE__ << endl;
      msg << "line: " << __LINE__ << endl;
      throw length_error(msg.str());
   }
   d_frame_body.push_back(d & 0x2);
   d_frame_body.push_back(d & 0x1);
}

size_t
abstract_data_unit::decode(size_t msg_sz, uint8_t *msg, imbe_decoder& imbe, float_queue& audio)
{
   if(!is_complete()) {
      ostringstream msg;
      msg << "cannot decode frame body - frame is not complete" << endl;
      msg << "(size now: "  << frame_size() << ", expected size: " << frame_size_max() << ")" << endl;
      msg << "func: " << __PRETTY_FUNCTION__ << endl;
      msg << "file: " << __FILE__ << endl;
      msg << "line: " << __LINE__ << endl;
      throw logic_error(msg.str());
   }
   if(msg_sz != data_size()) {
      ostringstream msg;
      msg << "cannot decode frame body ";
      msg << "(msg size: "  << msg_sz << ", expected size: " << data_size() << ")" << endl;
      msg << "func: " << __PRETTY_FUNCTION__ << endl;
      msg << "file: " << __FILE__ << endl;
      msg << "line: " << __LINE__ << endl;
      throw length_error(msg.str());
   }
   correct_errors(d_frame_body);
   decode_audio(d_frame_body, imbe, audio);
   return decode_body(d_frame_body, msg_sz, msg);
}

bool
abstract_data_unit::is_complete() const
{
   return d_frame_body.size() >= frame_size_max();
}

std::string
abstract_data_unit::snapshot() const
{
   string empty;
   return empty;
}

void
abstract_data_unit::dump(ostream& os) const
{
   uint32_t nbits = d_frame_body.size();
   os << setw(4) << nbits << " ";
   for(size_t i = 48; i < nbits; ++i) {
      os << (d_frame_body[i] ? "#" : "-");
   }
   os << endl;
}

abstract_data_unit::abstract_data_unit(const_bit_queue& frame_body) :
   d_frame_body(frame_body.size())
{
   copy(frame_body.begin(), frame_body.end(), d_frame_body.begin());
}

void
abstract_data_unit::correct_errors(bit_vector& frame_body)
{
}

size_t
abstract_data_unit::decode_audio(const_bit_vector& frame_body, imbe_decoder& imbe, float_queue& audio)
{
   return 0;
}

size_t
abstract_data_unit::decode_body(const_bit_vector& frame_body, size_t msg_sz, uint8_t *msg)
{
   return extract(frame_body, 0, static_cast<int>(frame_body.size()), msg);
}

const_bit_vector& 
abstract_data_unit::frame_body() const
{
   return d_frame_body;
}

uint16_t 
abstract_data_unit::frame_size_max() const
{
   return frame_size_max();
}

uint16_t 
abstract_data_unit::frame_size() const
{
   return d_frame_body.size();
}
