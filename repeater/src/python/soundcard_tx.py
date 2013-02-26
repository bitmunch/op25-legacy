#!/usr/bin/env python
#
# Copyright 2005,2006,2007 Free Software Foundation, Inc.
# 
# GNU Radio Multichannel APCO P25 Tx (c) Copyright 2009-2013 KA1RBI
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
# 

"""
Transmit 1 narrow band P25 signal using soundcard TX

"""

from gnuradio import gr, eng_notation, repeater
from gnuradio import audio
from gnuradio import blks2
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import math
import sys
import os

from gnuradio.wxgui import stdgui2, scopesink2
import wx

class tx_channel_soundcard(gr.hier_block2):
    def __init__(self, port, gain, soundcard_rate):
        gr.hier_block2.__init__(self, "tx_channel_soundcard",
                                gr.io_signature(0, 0, 0),
                                gr.io_signature(1, 1, gr.sizeof_float))

        msg_queue = gr.msg_queue(2)
        do_imbe = 1
        do_float = 1
        do_complex = 0
        if soundcard_rate == 96000:
		decim = 1
	else:
		decim = 2	# for 48000
        # per-channel GR source block including these steps:
        # - receive audio chunks from asterisk via UDP
        # - imbe encode
        # - generate float output stream (table lookup method)
        # - generates zero power while no input received
        self.chan = repeater.chan_usrp(port, do_imbe, do_complex, do_float, 1.0, decim, msg_queue)
        self.amp = gr.multiply_const_ff(gain)

        self.connect (self.chan, self.amp, self)

class fm_tx_block(stdgui2.std_top_block):
    def __init__(self, frame, panel, vbox, argv):
        MAX_CHANNELS = 1
        stdgui2.std_top_block.__init__ (self, frame, panel, vbox, argv)

        parser = OptionParser (option_class=eng_option)
        parser.add_option("-A", "--audio-output", type="string", default="")
        parser.add_option("-d","--debug", action="store_true", default=False)
        parser.add_option("-e","--enable-scope", action="store_true", default=False,
                          help="enable c4fm scope")
        parser.add_option("-g", "--gain", type="eng_float", default=1.0,
                          help="output gain (default: %default)")
        parser.add_option("-p", "--udp-port", type="int", default=32001,
                           help="UDP port number")
        parser.add_option("-s", "--output-sample-rate", type="int", default=48000,
                           help="soundcard output sampling rate")
        (options, args) = parser.parse_args ()

        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        # ----------------------------------------------------------------
        # Set up constants and parameters

        self.soundcard = audio.sink(options.output_sample_rate, options.audio_output)

        # Instantiate TX channel
        t = tx_channel_soundcard(options.udp_port, options.gain, options.output_sample_rate)
        self.connect (t, self.soundcard)

        # plot an FFT to verify we are sending what we want
        if options.enable_scope:
            post_mod = scopesink2.scope_sink_f(panel, title="C4FM Scope",
                                           fft_size=512, sample_rate=options.output_sample_rate,
                                           v_scale=5, t_scale=0.001)
            self.connect (t, post_mod)
            vbox.Add (post_mod.win, 1, wx.EXPAND)

        if options.debug:
        #    self.debugger = tx_debug_gui.tx_debug_gui(self.subdev)
        #    self.debugger.Show(True)
          print "attach pid %d" % os.getpid()
          raw_input("press enter")


def main ():
    sys.stderr.write("GNU Radio Multichannel APCO P25 Tx (c) Copyright 2009-2013 KA1RBI\n")
    app = stdgui2.stdapp(fm_tx_block, "Multichannel APCO P25 Tx", nstatus=1)
    app.MainLoop ()

if __name__ == '__main__':
    main ()
