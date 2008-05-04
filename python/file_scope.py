#!/usr/bin/env python

from gnuradio import gr, gru
from gnuradio import eng_notation
from gnuradio.eng_option import eng_option
from gnuradio.wxgui import stdgui, fftsink, waterfallsink, scopesink, form, slider
from optparse import OptionParser
import wx
import sys

class app_flow_graph(stdgui.gui_flow_graph):

    def __init__(self, frame, panel, vbox, argv):
        stdgui.gui_flow_graph.__init__(self)

        self.frame = frame
        self.panel = panel
        
        parser = OptionParser(option_class=eng_option)
        parser.add_option("-d", "--decim", type="int", default=256, help="set decimation")
        parser.add_option("-i", "--input", default="baseband.dat", help="specify input file")
        parser.add_option("-f", "--frequency", type="eng_float", default=None, help="set center frequency")
        parser.add_option("-b", "--bandwidth", type="eng_float", default=12.5e3, help="set bandwidth")
        parser.add_option("-r", "--repeat", action="store_true", default=False, help="repeat in a loop")
        parser.add_option("-s", "--scale", type="int", default=1, help="FFT scale factor")
        parser.add_option("-W", "--waterfall", action="store_true", default=False, help="enable waterfall display")
        parser.add_option("-R", "--float-scope", action="store_true", default=False, help="enable float oscilloscope")
        parser.add_option("-C", "--complex-scope", action="store_true", default=False, help="enable complex oscilloscope")
        parser.add_option("-F", "--fft", action="store_false", default=True, help="enable FFT")
        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        usrp_rate = 64000000
        sample_rate = usrp_rate // options.decim

        FILE = gr.file_source(gr.sizeof_gr_complex, options.input, options.repeat)
        last = FILE

        THROTTLE = gr.throttle(gr.sizeof_gr_complex, sample_rate);
        self.connect(last, THROTTLE)
        last = THROTTLE

        if options.frequency:
            coeffs = gr.firdes.low_pass(1.0, sample_rate, options.bandwidth/2, options.bandwidth/2, gr.firdes.WIN_HANN)
            FILT = gr.freq_xlating_fir_filter_ccf(1, coeffs, -options.frequency, sample_rate)
            self.connect(last, FILT)
            last = FILT

        if options.fft:
            decim_rate = sample_rate//options.scale;
            DECIM = gr.keep_one_in_n(gr.sizeof_gr_complex, options.scale)
            SCOPE = fftsink.fft_sink_c (self, panel, fft_size=512, sample_rate=decim_rate)
            vbox.Add(SCOPE.win, 10, wx.EXPAND)
            self.connect(last, DECIM, SCOPE)
            # wx.lib.evtmgr.eventManager.Register(self.mouse, wx.EVT_LEFT_DOWN, SCOPE.win)
            # self.Bind(wx.EVT_LEFT_DOWN, self.on_left_click)
        elif options.waterfall:
            SCOPE = waterfallsink.waterfall_sink_c (self, panel, fft_size=1024, sample_rate=sample_rate)
            vbox.Add(SCOPE.win, 10, wx.EXPAND)
            self.connect(last, SCOPE)
        elif options.complex_scope:
            SCOPE = scopesink.scope_sink_c(self, panel, sample_rate=sample_rate) 
            vbox.Add(SCOPE.win, 10, wx.EXPAND)
            self.connect(last, SCOPE)
        if options.float_scope:
            CONV = gr.complex_to_float()
            SCOPE = scopesink.scope_sink_f(self, panel, sample_rate=sample_rate)          
            vbox.Add(SCOPE.win, 10, wx.EXPAND)
            self.connect(last, CONV, SCOPE)
        last = SCOPE

    def on_left_click(self, event):
#         fRel = (event.GetX() - 48) / 15.2 - 20;
#         print eng_notation.num_to_str(self.frequency(fRel*1e3))
         print "here"

def main ():
    app = stdgui.stdapp(app_flow_graph, "Signal Scope", nstatus=1)
    app.MainLoop()

if __name__ == '__main__':
    main ()
