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

#ifndef INCLUDED_SNIFFER_DU_HANDLER_H
#define INCLUDED_SNIFFER_DU_HANDLER_H

#include <data_unit_handler.h>
#include <string>

/**
 * sniffer_du_handler interface. Writes each data_unit to a TUN/TAP device.
 */
class sniffer_du_handler : public data_unit_handler
{

public:

   /**
    * sniffer_du_handler constructor.
    *
    * \param next The next data_unit_handler.
    */
   explicit sniffer_du_handler(data_unit_handler_sptr next);

   /**
    * sniffer_du_handler virtual destructor.
    */
   virtual ~sniffer_du_handler();

   /**
    * Handle a received P25 frame.
    *
    * \param du A non-null data_unit_sptr to handle.
    */
   virtual void handle(data_unit_sptr du);

   /**
    * Return a pointer to a string naming the TUN/TAP device.
    *
    * \return A pointer to a NUL-terminated character string.
    */
   const char *device_name() const;

private:

   /**
    * file descriptor to the TUN/TAP device.
    */
   int32_t d_tap;

   /**
    * A string naming the TUN/TAP device.
    */
   std::string d_tap_device;

};

#endif /* INCLUDED_SNIFFER_DU_HANDLER_H */
