/* -*- C++ -*- */

/*
 * Copyright 2008, 2009 Steve Glass
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

#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <sniffer_du_handler.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

sniffer_du_handler::sniffer_du_handler(data_unit_handler_sptr next) :
   data_unit_handler(next),
   d_tap(-1),
   d_tap_device("unavailable")
{
   d_tap = open("/dev/net/tun", O_WRONLY);
   if(-1 != d_tap) {
      struct ifreq ifr;
      memset(&ifr, 0, sizeof(ifr));
      ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
      strncpy(ifr.ifr_name, "p25-%d", IFNAMSIZ);
      if(0 == ioctl(d_tap, TUNSETIFF, &ifr)) {
         // force the device into the UP state
         int s = socket(AF_INET, SOCK_DGRAM, 0);
         if(0 <= s) {
            if(0 == ioctl(s, SIOCGIFFLAGS, &ifr)) {
               ifr.ifr_flags = IFF_UP;
               if(0 == ioctl(s, SIOCSIFFLAGS, &ifr)) {
                  d_tap_device = ifr.ifr_name;
               } else {
                  perror("ioctl(d_tap, SIOCSIFFLAGS, &ifr)");
                  close(d_tap);
                  d_tap = -1;
               }
            } else {
               perror("ioctl(d_tap, SIOCGIFFLAGS, &ifr)");
               close(d_tap);
               d_tap = -1;
            }
            close(s);
         } else {
            perror("socket(AF_INET, SOCK_DGRAM, 0)");
            close(d_tap);
            d_tap = -1;           
         }
      } else {
         perror("ioctl(d_tap, TUNSETIFF, &ifr)");
         close(d_tap);
         d_tap = -1;
      }
   }
}

sniffer_du_handler::~sniffer_du_handler()
{
   if(-1 != d_tap) {
      close(d_tap);
   }
}

const char*
sniffer_du_handler::device_name() const
{
   return d_tap_device.c_str();
}

void
sniffer_du_handler::handle(data_unit_sptr du)
{
   if(-1 != d_tap) {
      const size_t du_sz = du->size();
      const size_t tap_hdr_sz = 14;
      const size_t tap_sz = du_sz + tap_hdr_sz;
      uint8_t tap[tap_sz];
      memset(&tap[0], 0x00, 6);
      memset(&tap[6], 0x00, 6);
      memset(&tap[12], 0xff, 2);
      du->decode_frame(du_sz, &tap[tap_hdr_sz]);
      write(d_tap, tap, tap_sz);
   }
   data_unit_handler::handle(du);
}
