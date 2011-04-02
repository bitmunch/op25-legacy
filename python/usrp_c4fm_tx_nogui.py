#!/usr/bin/env python
#
# Copyright 2011 Steve Glass.
# 
# This file is part of GNU Radio and part of OP25
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# It is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

import math
import sys
import wx

from gnuradio import audio
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import p25_mod_bf
from gnuradio import usrp
from gnuradio.eng_option import eng_option

# Python is putting the packages in some strange places
# This is a workaround until we figure out WTF is going on
try:
    from gnuradio import fsk4, op25
except Exception:
    import fsk4, op25


"""
Simple tx-from-pcap-file utility.
"""
class usrp_c4fm_tx_block(gr.top_block):

    def __init__(self, subdev_spec, freq, subdev_gain, filename, delay, repeat):

        gr.top_block.__init__ (self)

        # important vars (to be calculated from USRP when available
        dac_rate = 128e6
        usrp_interp = 400
        usrp_rate = dac_rate / usrp_interp # 320KS/s
        sw_interp = 10
        channel_rate = usrp_rate / sw_interp  # 32 kS/s

        # open the pcap source
        pcap = op25.pcap_source_b(filename, delay, repeat)
        # ToDo: consider octet -> symbol unpacking in flow graph

        c4fm = p25_mod_bf.p25_mod(output_sample_rate=channel_rate, log=False, verbose=False)
        self.connect(pcap, c4fm)

        # setup low pass filter + interpolator
        interp_factor = self.usrp_rate / self.channel_rate
        low_pass = 2.88e3
        interp_taps = gr.firdes.low_pass(1.0, self.usrp_rate, low_pass, low_pass * 0.1, gr.firdes.WIN_HANN)
        interpolator = gr.interp_fir_filter_fff (int(interp_factor), interp_taps)
        self.connect(c4fm, interpolator)

        # frequency modulator
        max_dev = 12.5e3
        k = 2 * math.pi * max_dev / self.usrp_rate
        adjustment = 1.5   # adjust for proper c4fm deviation level
        fm = gr.frequency_modulator_fc(k * adjustment)
        self.connect(interpolator, fm)

        # amplify signal
        gain = gr.multiply_const_cc(4000)
        self.connect(fm, gain)

        # # configure USRP
        u = usrp.sink_c()
        if subdev_spec is None:
            subdev_spec = usrp.pick_tx_subdevice(u)
        u.set_mux(usrp.determine_tx_mux_value(u, subdev_spec))
        subdev = usrp.selected_subdev(u, subdev_spec)
        print "Using TX d'board %s" % (subdev.side_and_name(),)

        if gain is None:
            g = subdev.gain_range()
            gain = float(g[0] + g[1]) / 2
        subdev.set_gain(self.subdev.gain_range()[0])
        u.tune(subdev.which(), subdev, freq)
        self.connect(gain, u)
        subdev.set_enable(True)

#        sink = gr.null_sink(gr.sizeof_float)
#        self.connect(gain, sink)


# inject frames
#
if __name__ == '__main__':

    from optparse import OptionParser
    parser = OptionParser (option_class=eng_option)
    parser.add_option("-T", "--tx-subdev-spec", type="subdev", default=None,
                      help="select USRP Tx side A or B")
    parser.add_option("-f", "--freq", type="eng_float", default=None,
                      help="set Tx frequency to FREQ [required]")
    parser.add_option("-g", "--gain", type="eng_float", default=None, 
                      help="set device gain")
    parser.add_option("-p", "--pcap-file", default="",
                      help="packet capture file [required]")
    parser.add_option("-d", "--delay", type="int", default=0,
                      help="delay before TX of first symbol [default=%default]")
    parser.add_option("-r","--repeat", action="store_true", default=False,
                      help="continuously replay input file [default=%default]")
    (options, args) = parser.parse_args ()
    if options.freq is None:
        parser.print_help()
        sys.exit(1)
    
    try:
        rx = usrp_c4fm_tx_block(options.subdev_spec, options.freq, options.gain, options.pcap_file, options.delay, options.repeat)
        rx.run()
    except KeyboardInterrupt:
        rx.subdev.set_enable(False)
