#!/usr/bin/env python

# Copyright 2008 Steve Glass
# 
# This file is part of OP25.
# 
# OP25 is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# OP25 is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
# License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with OP25; see the file COPYING. If not, write to the Free
# Software Foundation, Inc., 51 Franklin Street, Boston, MA
# 02110-1301, USA.

import sys
# import threading
import wx

from gnuradio import gr, gru, optfir, fsk4
from gnuradio import eng_notation
from gnuradio import op25
from gnuradio.eng_option import eng_option
from gnuradio.wxgui import stdgui2, fftsink2, scopesink2
from math import pi
from optparse import OptionParser
from usrpm import usrp_dbid

class p25_rx_block (stdgui2.std_top_block):

    def __init__(self, frame, panel, vbox, argv):
        stdgui2.std_top_block.__init__(self, frame, panel, vbox, argv)

        self.frame = frame
        self.frame.CreateStatusBar()
        self.frame.SetStatusText("")
        self.panel = panel

        parser = OptionParser(option_class=eng_option)
        # either these
        parser.add_option("-i", "--input", default=None, help="set input file")
        parser.add_option("-d", "--decim", type="int", default=256, help="set decimation")
        parser.add_option("-l", "--loop", action="store_true", default=False, help="loop input file")
        # or these
        parser.add_option("-f", "--frequency", type="eng_float", default=0.0, help="set channel frequency", metavar="Hz")
        parser.add_option("-b", "--bandwidth", type="eng_float", default=12.5e3, help="set bandwidth")
        parser.add_option("-s", "--channel-decim", type="int", default=1, help="channel decimation")
        # not used at present....
        parser.add_option("-o", "--output", type="string", default="audio.dat", help="output file")
        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        usrp_rate = 64000000
        source_rate = usrp_rate // options.decim

        if options.input:
            file = gr.file_source(gr.sizeof_gr_complex, options.input, options.loop)
            throttle = gr.throttle(gr.sizeof_gr_complex, source_rate);
            self.connect(file, throttle)
            self.source = throttle
        elif options.frequency:
            usrp = usrp.source_c(0)
            subdev = usrp.pick_subdev(usrp, (usrp_dbid.TV_RX, usrp_dbid.TV_RX_REV_2, udrp.dbid.TV_RX_REV_3))
            usrp.set_mux(usrp.determine_rx_mux_value(usrp, subdev))
            usrp.set_decim_rate(options.decim)
#             if gain is None:
#                 g = self._subdev.gain_range()
#                 gain = (g[0]+g[1])/2.0
#             self._subdev.set_gain(gain)
            usrp.tune(options.frequency)
            junk = usrp.tune(usrp, 0, subdev, frequency)
        else:
            print "no input specified!"
            exit(1)

        self.spectrum = fftsink2.fft_sink_c(self.panel, fft_size=512, sample_rate=source_rate, title="RF Spectrum", average=False, peak_hold=False)
        self.connect(self.source, self.spectrum)
        vbox.Add(self.spectrum.win, 4, wx.EXPAND)
        self.spectrum.win.plot.Bind(wx.EVT_LEFT_DOWN, self.spectrum_on_left_click)

        self.offset = 0.0
#         channel_decim = 1
#         channel_rate = source_rate
        channel_decim = source_rate // 125000
        channel_rate = source_rate // channel_decim
        trans_width = options.bandwidth / 2;
        trans_centre = trans_width + (trans_width / 2)
        coeffs = gr.firdes.low_pass(1.0, source_rate, trans_centre, trans_width, gr.firdes.WIN_HANN)
        self.channel_filter = gr.freq_xlating_fir_filter_ccf(channel_decim, coeffs, -options.frequency, source_rate)
        self.connect(self.source, self.channel_filter);

        self.symbol_rate = 4800
        self.symbol_deviation = 600.0
        fm_demod_gain = channel_rate / (2.0 * pi * self.symbol_deviation)
        self.fm_demod = gr.quadrature_demod_cf(fm_demod_gain)
        self.connect(self.channel_filter, self.fm_demod)

        symbol_decim = 1
        symbol_coeffs = gr.firdes.root_raised_cosine(61.0, channel_rate, self.symbol_rate, 0.2, 500)
        self.symbol_filter = gr.fir_filter_fff (symbol_decim, symbol_coeffs)
        self.connect(self.fm_demod, self.symbol_filter)

        self.msgq = gr.msg_queue(2)
        self.demod_fsk4 = fsk4.demod_ff(self.msgq, channel_rate, self.symbol_rate)
        self.connect(self.symbol_filter, self.demod_fsk4)

        self.p25_decoder = op25.decoder_ff(self.msgq)
        self.connect(self.demod_fsk4, self.p25_decoder)

        self.sink = gr.null_sink(gr.sizeof_float)
        self.connect(self.p25_decoder, self.sink)

    def spectrum_on_left_click(self, event):
        chanWid = 12.5e3
        x, y = self.spectrum.win.plot.GetXY(event)
        # ugly kludge to cope when units shift from KHz to MHz
        if x < 1.0:
            x *= 1e6
        elif x < 100.0:
            x *= 1e3
        # ToDo: query the scope for the nearest unit
        x = (x // chanWid) * chanWid
        self.channel_filter.set_center_freq(-x)
        self.frame.SetStatusText(str(x))

if '__main__' == __name__:
    app = stdgui2.stdapp(p25_rx_block, "APCO P25 Receiver")
    app.MainLoop()
