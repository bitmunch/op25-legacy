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

#include <cstdlib>
#include <cerrno>
#include <sstream>
#include <fcntl.h>
#include <stdint.h>
#include <swab.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <vc55_imbe_decoder.h>

using namespace std;

vc55_imbe_decoder::vc55_imbe_decoder()
{
   const char *dev = getenv("IMBE_DEV");
   const char *default_dev = "/dev/ttyS0";
   dev = dev ? dev : default_dev;
   d_fd = open(dev, O_WRONLY | O_NOCTTY);
   if(-1 != d_fd) {
      struct termios options;
      if(-1 != tcgetattr(d_fd, &options)) {
         cfsetospeed(&options, B115200);
         options.c_cflag |= (CLOCAL | CREAD | CS8);
         options.c_cflag &= ~(CSTOPB | PARENB | CRTSCTS | IXON);
         if(-1 == tcsetattr(d_fd, TCSANOW, &options)) {
            ostringstream msg;
            msg << "tcsetattr(fd, TCSANOW, &options): " << strerror(errno) << endl;
            msg << "func: " << __PRETTY_FUNCTION__ << endl;
            msg << "file: " << __FILE__ << endl;
            msg << "line: " << __LINE__ << endl;
            throw runtime_error(msg.str());
         }
      }
   } else {
      perror("open(dev, O_WRONLY | O_NOCTTY)"); // a warning, not an error
   }
}

vc55_imbe_decoder::~vc55_imbe_decoder()
{
   if(-1 != d_fd) {
      close(d_fd);
   }
}

size_t
vc55_imbe_decoder::decode(voice_codeword& in_out, audio_output& out)
{
   if(-1 != d_fd) {
      uint8_t packet[20];
      packet[0] = 0x56;
      packet[1] = 0xf0;

      swab(in_out, 0, 144, &packet[2]);
      if(-1 == write(d_fd, packet, sizeof(packet))) {
         perror("write(d_fd, packet, sizeof(packet))");
         close(d_fd);
         d_fd = -1;
      }
   }
   return 0;
}
