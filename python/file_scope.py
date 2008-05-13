#!/usr/bin/env python

import wx
import sys

from gnuradio import gr, gru
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from gnuradio.wxgui import stdgui, fftsink, waterfallsink, scopesink #, form
from optparse import OptionParser

class app_flow_graph(stdgui.gui_flow_graph):

    def __init__(self, frame, panel, vbox, argv):
        stdgui.gui_flow_graph.__init__(self)

        parser = OptionParser(option_class=eng_option)
        parser.add_option("-d", "--decim", type="int", default=256, help="set decimation")
        parser.add_option("-i", "--input", default="baseband.dat", help="set input file")
        parser.add_option("-l", "--loop", action="store_true", default=False, help="loop input file")
        parser.add_option("-b", "--bandwidth", type="eng_float", default=None, help="set bandwidth")
        parser.add_option("-f", "--frequency", type="eng_float", default=0.0, help="set channel frequency", metavar="Hz")
        parser.add_option("-r", "--rf-squelch", type="eng_float", default=None, help="RF squelch (in dB)", metavar="dB")
        parser.add_option("-s", "--channel-decim", type="int", default=1, help="channel decimation")
        parser.add_option("-o", "--output", type="string", default=None, help="output file")
 
        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        usrp_rate = 64000000
        sample_rate = usrp_rate // options.decim

        FILE = gr.file_source(gr.sizeof_gr_complex, options.input, options.loop)
        last = FILE

        THROTTLE = gr.throttle(gr.sizeof_gr_complex, sample_rate);
        self.connect(last, THROTTLE)
        last = THROTTLE

        self.SCOPE1 = fftsink.fft_sink_c(self, panel, fft_size=512, sample_rate=sample_rate)
        vbox.Add(self.SCOPE1.win, 10, wx.EXPAND)
        self.SCOPE1.win.Bind(wx.EVT_LEFT_DOWN, self.scope1_on_left_click)
        self.connect(last, self.SCOPE1)

        if options.bandwidth:
            coeffs = gr.firdes.low_pass(1.0, sample_rate, options.bandwidth/2, options.bandwidth/2, gr.firdes.WIN_HANN)
            self.FILT = gr.freq_xlating_fir_filter_ccf(options.channel_decim, coeffs, -options.frequency, sample_rate)
            self.connect(last, self.FILT)
            last = self.FILT
            if options.rf_squelch:
                RFSQL = gr.pwr_squelch_cc(options.rf_squelch, 1e-3, 0, True)
                self.connect(last, RFSQL)
                last = RFSQL

        if options.output:
            OUT = gr.file_sink(gr.sizeof_gr_complex, options.output)
            self.connect(last, OUT)

#         SCOPE2 = fftsink.fft_sink_c(self, panel, fft_size=512, sample_rate=sample_rate)
#         vbox.Add(SCOPE2.win, 11, wx.EXPAND)
#         self.connect(last, SCOPE2)

#         SCOPE2 = scopesink.scope_sink_c(self, panel, sample_rate=sample_rate) 
#         vbox.Add(SCOPE2.win, 12, wx.EXPAND)
#         self.connect(last, SCOPE2)

        CONV = gr.complex_to_float()
        SCOPE2 = scopesink.scope_sink_f(self, panel, sample_rate=sample_rate)         
        vbox.Add(SCOPE2.win, 11, wx.EXPAND)
        self.connect(last, CONV, SCOPE2)

    def scope1_on_left_click(self, event):
        chanWid = 12.5e3
        x, y = self.SCOPE1.win.GetXY(event)
        x = ((x * 1e6) // chanWid) * chanWid
        self.FILT.set_center_freq(-x)

def main ():
    app = stdgui.stdapp(app_flow_graph, "Signal Scope", nstatus=1)
    app.MainLoop()

if __name__ == '__main__':
    main ()
