#!/usr/bin/env python
#
# Copyright 2005,2006,2007 Free Software Foundation, Inc.
# 
# Copyright 2010 KA1RBI
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

from gnuradio import gr, gru, eng_notation, optfir
from gnuradio import usrp
from gnuradio import repeater
from gnuradio import blks2
from gnuradio.eng_option import eng_option
from optparse import OptionParser
from usrpm import usrp_dbid
import sys
import os
import math
from math import pi

"""
USRP multichannel RX app

Simultaneously receives multiple p25 stations,
decoded audio is written over multiple parallel channels to asterisk app_rpt,
or optionally to wireshark [if -w option specified]
"""

# edit next two lines - set the center freq halfway between lowest and highest
center_freq = 851.8125e6
chans = [851.3125e6, 851.4125e6, 851.9125e6, 852.0125e6, 852.3125e6]

WIRESHARK_PORT = 23456

def pick_subdevice(u):
    """
    The user didn't specify a subdevice on the command line.
    Try for one of these, in order: TV_RX, BASIC_RX, whatever is on side A.

    @return a subdev_spec
    """
    return usrp.pick_subdev(u, (usrp_dbid.TV_RX,
                                usrp_dbid.TV_RX_REV_2,
				usrp_dbid.TV_RX_REV_3,
				usrp_dbid.DBS_RX,
                                usrp_dbid.BASIC_RX))

class rx_channel_cqpsk(gr.hier_block2):
    def __init__(self, sps, channel_decim, channel_taps, options, usrp_rate, channel_rate, lo_freq, port):
        gr.hier_block2.__init__(self, "rx_channel_cqpsk",
                                gr.io_signature(1, 1, gr.sizeof_gr_complex),
                                gr.io_signature(0, 0, 0))

        chan = gr.freq_xlating_fir_filter_ccf(int(channel_decim), channel_taps, lo_freq, usrp_rate)

        agc = gr.feedforward_agc_cc(16, 1.0)

        gain_omega = 0.125 * options.gain_mu * options.gain_mu

        alpha = options.costas_alpha
        beta = 0.125 * alpha * alpha
        fmin = -0.025
        fmax =  0.025

        clock = repeater.gardner_costas_cc(sps, options.gain_mu, gain_omega, alpha, beta, fmax, fmin)

        # Perform Differential decoding on the constellation
        diffdec = gr.diff_phasor_cc()

        # take angle of the difference (in radians)
        to_float = gr.complex_to_arg()

        # convert from radians such that signal is in -3/-1/+1/+3
        rescale = gr.multiply_const_ff( (1 / (pi / 4)) )

        # reduce float symbols to binary dibits
        levels = [ -2.0, 0.0, 2.0, 4.0 ]
        slicer = repeater.fsk4_slicer_fb(levels)

        self.connect (self, chan, agc, clock, diffdec, to_float, rescale, slicer)
        if options.wireshark:
            # build p25 frames from raw dibits and write to wireshark
            msgq = gr.msg_queue(2)
            decoder = repeater.p25_frame_assembler(options.hostname, WIRESHARK_PORT, options.debug, False, False, False, msgq)
            self.connect (slicer, decoder)
        else:
            # build p25 frames from raw dibits and extract IMBE speech codewords
            msgq = gr.msg_queue(2)
            decoder = repeater.p25_frame_assembler('', 0, options.debug, True, True, False, msgq)
    
            # decode the IMBE codewords - outputs speech at 8k rate
            imbe = repeater.vocoder(False, False, 0, "", 0, False)

            # write the audio (8k, signed int16) to asterisk app_rpt via UDP
            chan_rpt = repeater.chan_usrp_rx(options.hostname, port, options.debug)

            self.connect (slicer, decoder, imbe, chan_rpt)

class usrp_rx_block (gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self)

        parser=OptionParser(option_class=eng_option)
        parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=None,
                          help="select USRP Rx side A or B (default=A)")
        parser.add_option("-C", "--costas-alpha", type="eng_float", default=0.10, help="offset frequency", metavar="Hz")
        parser.add_option("-c", "--calibration", type="int", default=0,
                          help="USRP calibration offset", metavar="FREQ")
        parser.add_option("-d","--debug", type="int", default=0, help="debug level")
        parser.add_option("-G", "--gain-mu", type="eng_float", default=0.05, help="Gardner gain")
        parser.add_option("-H", "--hostname", type="string", default="127.0.0.1",
                          help="IP address of asterisk (or wireshark) host")
        parser.add_option("-g", "--gain", type="eng_float", default=None,
                          help="set gain in dB (default is midpoint)")
        parser.add_option("-p", "--port", type="int", default=32001,
                          help="starting port number for chan_usrp")
        parser.add_option("-w", "--wireshark", action="store_true", default=False, help="write data to wireshark")

        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        if options.debug > 10:
            print 'Ready for GDB to attach (pid = %d)' % (os.getpid(),)
            raw_input("Press 'Enter' to continue...")
        
        # build graph
        self.u = usrp.source_c()                    # usrp is data source

        adc_rate = self.u.adc_rate()                # 64 MS/s
        usrp_decim = 60
        self.u.set_decim_rate(usrp_decim)
        usrp_rate = adc_rate / usrp_decim           # 1,066,667

        channel_decim = 50.0                        # NB: 100 causes trouble
        channel_rate = usrp_rate / channel_decim
        sps = channel_rate / 4800.0
        print "channel rate %f sps %f" % (channel_rate, sps)

        low_pass = 15e3
        channel_taps = gr.firdes.low_pass(1.0, usrp_rate, low_pass, low_pass * 0.1, gr.firdes.WIN_HANN)

        if options.rx_subdev_spec is None:
            options.rx_subdev_spec = pick_subdevice(self.u)

        self.u.set_mux(usrp.determine_rx_mux_value(self.u, options.rx_subdev_spec))
        self.subdev = usrp.selected_subdev(self.u, options.rx_subdev_spec)
        print "Using RX d'board %s" % (self.subdev.side_and_name(),)

        for i in range(len(chans)):
            lo_freq = center_freq - chans[i]
            t = rx_channel_cqpsk(sps, channel_decim, channel_taps, options, usrp_rate, channel_rate, lo_freq, options.port + i)
            self.connect(self.u, t)
            print "attaching channel %d freq %f port %d" % (i+1, lo_freq, options.port + i)

        if options.gain is None:
            # if no gain was specified, use the mid-point in dB
            g = self.subdev.gain_range()
            options.gain = float(g[0]+g[1])/2

        usrp_freq = center_freq + options.calibration

        # set initial values

        self.set_gain(options.gain)

        if not(self.set_freq(usrp_freq)):
            self._set_status_msg("Failed to set initial frequency")

    def set_freq(self, target_freq):
        """
        Set the center frequency we're interested in.

        @param target_freq: frequency in Hz
        @rypte: bool

        Tuning is a two step process.  First we ask the front-end to
        tune as close to the desired frequency as it can.  Then we use
        the result of that operation and our target_frequency to
        determine the value for the digital down converter.
        """
        r = self.u.tune(0, self.subdev, target_freq)
        
        if r:
            self.freq = target_freq
            return True

        return False

    def set_gain(self, gain):
        self.subdev.set_gain(gain)

if __name__ == '__main__':
    sys.stderr.write("GNU Radio Multichannel APCO P25 Rx (c) Copyright 2009,2010 KA1RBI\n")
    tb = usrp_rx_block()
    try:
        tb.run()
    except KeyboardInterrupt:
        pass
