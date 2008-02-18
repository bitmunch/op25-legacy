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
        parser.add_option("-r", "--repeat", action="store_true", default=False, help="repeat in a loop")
        parser.add_option("-W", "--waterfall", action="store_true", default=False, help="enable waterfall display")
        parser.add_option("-S", "--oscilloscope", action="store_true", default=False, help="enable simple oscilloscope")
        parser.add_option("-c", "--complex-scope", action="store_true", default=False, help="enable complex oscilloscope")
        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        usrp_rate = 64000000
        sample_rate = usrp_rate // options.decim

        FILE = gr.file_source(gr.sizeof_gr_complex, options.input, options.repeat)

        THROTTLE = gr.throttle(gr.sizeof_gr_complex, sample_rate);

        if options.waterfall:
            SCOPE = waterfallsink.waterfall_sink_c (self, panel, fft_size=1024, sample_rate=sample_rate)
            self.connect(FILE, THROTTLE, SCOPE)
        elif options.complex_scope:
            SCOPE = scopesink.scope_sink_c(self, panel, sample_rate=sample_rate)
            self.connect(FILE, THROTTLE, SCOPE)
        elif options.oscilloscope:
            CONV = gr.complex_to_float()
            SCOPE = scopesink.scope_sink_f(self, panel, sample_rate=sample_rate)          
            self.connect(FILE, THROTTLE, CONV, SCOPE)
        else:
            SCOPE = fftsink.fft_sink_c (self, panel, fft_size=1024, sample_rate=sample_rate)
            self.connect(FILE, THROTTLE, SCOPE)

        vbox.Add(SCOPE.win, 10, wx.EXPAND)

def main ():
    app = stdgui.stdapp(app_flow_graph, "Signal Scope", nstatus=1)
    app.MainLoop()

if __name__ == '__main__':
    main ()
