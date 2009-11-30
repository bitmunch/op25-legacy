#!/usr/bin/env python
#
# Copyright 2005,2006,2007 Free Software Foundation, Inc.
# 
# GNU Radio Multichannel APCO P25 Tx (c) Copyright 2009, KA1RBI
# 
# This file is part of GNU Radio and part of OP25
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# It is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

"""
This program is used in conjunction with op25_tx.py.

When op25_tx.py is used with its -p option (selecting UDP input),
you should start this program beforehand, to start the data flow.

"""

import math
from gnuradio import gr, gru, audio, eng_notation, blks2, optfir
from gnuradio.eng_option import eng_option
from optparse import OptionParser

from gnuradio import op25_imbe

class app_top_block(gr.top_block):
    def __init__(self, options, queue):
        gr.top_block.__init__(self, "mhp")

        self.audio_amps = []
        self.converters = []
        self.vocoders = []

        input_audio_rate = 8000
        self.audio_input = audio.source(input_audio_rate, options.audio_input)

        for i in range (options.nchannels):
            t = gr.multiply_const_ff(32767 * options.audio_gain)
            self.audio_amps.append(t)
            t = gr.float_to_short()
            self.converters.append(t)
            t = op25_imbe.vocoder(True,                 # 0=Decode,True=Encode
                                  options.verbose,      # Verbose flag
                                  options.stretch,      # flex amount
                                  options.udp_addr,     # udp ip address
                                  options.udp_port + i, # udp port
                                  False)                # dump raw u vectors
            self.vocoders.append(t)

        for i in range (options.nchannels):
            self.connect((self.audio_input, i), self.audio_amps[i], self.converters[i], self.vocoders[i])

def main():
    parser = OptionParser(option_class=eng_option)
    parser.add_option("-a", "--udp-addr", type="string", default="127.0.0.1", help="destination host IP address")
    parser.add_option("-g", "--audio-gain", type="eng_float", default=1.0, help="gain factor")
    parser.add_option("-n", "--nchannels", type="int", default=2, help="number of audio channels")
    parser.add_option("-p", "--udp-port", type="int", default=2525, help="destination host port")
    parser.add_option("-v", "--verbose", action="store_true", default=False, help="dump demodulation data")
    parser.add_option("-I", "--audio-input", type="string", default="", help="pcm input device name.  E.g., hw:0,0 or /dev/dsp")
    parser.add_option("-S", "--stretch", type="int", default=0)
    (options, args) = parser.parse_args()

 
    queue = gr.msg_queue()
    tb = app_top_block(options, queue)
    try:
        tb.start()
        while 1:
            if not queue.empty_p():
                sys.stderr.write("main: q.delete_head()\n")
                msg = queue.delete_head()
    except KeyboardInterrupt:
        tb.stop()

if __name__ == "__main__":
    main()
