#!/usr/bin/env python
from gnuradio import gr, gru, usrp, audio, eng_notation, blks
from gnuradio.eng_option import eng_option
from optparse import OptionParser

"""
This application samples a narrowband channel and writes complex
baseband data to file. To collect data only for transmissions the
channel is gated using a power squelch.
"""

class usrp_source_c(gr.hier_block):

    """
    Initialize the USRP.
    """

    def __init__(self, fg, subdev_spec, decim, gain=None):
        self._decim = decim
        self._src = usrp.source_c()
        if subdev_spec is None:
            subdev_spec = usrp.pick_rx_subdevice(self._src)
            self._subdev = usrp.selected_subdev(self._src, subdev_spec)
            print "Using RX d'board %s" % (self._subdev.side_and_name(),)
        self._src.set_mux(usrp.determine_rx_mux_value(self._src, subdev_spec))
        self._src.set_decim_rate(self._decim)
        if gain is None:
            g = self._subdev.gain_range()
            gain = (g[0]+g[1])/2.0
        self._subdev.set_gain(gain)
        gr.hier_block.__init__(self, fg, self._src, self._src)

    def tune(self, freq):
        r = usrp.tune(self._src, 0, self._subdev, freq)

class app_flow_graph(gr.flow_graph):
    def __init__(self, options):
        gr.flow_graph.__init__(self)

        usrp_rate = 64000000
        sample_rate = usrp_rate // options.decim
        
        if options.frequency:
            USRP = usrp_source_c(self, options.rx_subdev_spec, options.decim, options.gain)
            USRP.tune(options.frequency)
            IN = USRP
        elif options.input:
            FILE = gr.file_source(gr.sizeof_gr_complex, options.input)
            THROTTLE = gr.throttle(gr.sizeof_gr_complex, sample_rate);
            self.connect(FILE, THROTTLE)
            IN = THROTTLE
        else:
            print "error: no source selected"
            exit

        coeffs = gr.firdes.low_pass(1.0, sample_rate, options.low_pass, options.low_pass * 0.1, gr.firdes.WIN_HANN)
        FILT = gr.freq_xlating_fir_filter_ccf(1, coeffs, options.calibration, sample_rate)
        self.connect(IN, FILT)

        RFSQL = gr.pwr_squelch_cc(options.rf_squelch, 1e-3, 0, True)
        self.connect(FILT, RFSQL)

        OUT = gr.file_sink(gr.sizeof_gr_complex, options.output)
        self.connect(RFSQL, OUT)

def main():
    parser = OptionParser(option_class=eng_option)
    parser.add_option("-c", "--calibration", type="eng_float", default=0, help="calibration offset", metavar="Hz")
    parser.add_option("-d", "--decim", type="int", default=256, help="set decimation")
    parser.add_option("-f", "--frequency", type="eng_float", default=None, help="set receive frequency", metavar="Hz")
    parser.add_option("-g", "--gain", type="int", default=None, help="set RF gain", metavar="dB")
    parser.add_option("-i", "--input", type="string", default=None, help="the input file")
    parser.add_option("-l", "--low-pass", type="eng_float", default=25e3, help="low pass cut-off", metavar="Hz")
    parser.add_option("-o", "--output", type="string", default="baseband.dat", help="the output file")
    parser.add_option("-r", "--rf-squelch", type="eng_float", default=0.0, help="set RF squelch to dB", metavar="dB")
    parser.add_option("-R", "--rx-subdev-spec", type="subdev", help="select USRP Rx side A or B", metavar="SUBDEV")
    (options, args) = parser.parse_args()
    if options.frequency:
        if options.frequency < 1e6:
            options.frequency *= 1e6
    fg = app_flow_graph(options)
    try:
        fg.run()
    except KeyboardInterrupt:
        pass
 
if __name__ == "__main__":
    main()
 
