#!/usr/bin/env python
from gnuradio import gr, gru, usrp, audio, eng_notation, blks
from gnuradio.eng_option import eng_option
from optparse import OptionParser

"""
This app reads baseband data from file and demodulates that baseband
as a P25 DQPSK signal (in theory, this is sufficient to capture both
phase I and phase II signals).
"""

class app_flow_graph(gr.flow_graph):
    def __init__(self, options):
        gr.flow_graph.__init__(self)

        usrp_rate = 64000000
        sample_rate = usrp_rate // options.decim

        reqd_sample_rate = 57600
        symbol_rate = 4800

        IN = gr.file_source(gr.sizeof_gr_complex, options.input_file)

        coeffs = gr.firdes.low_pass (1.0, sample_rate, 11.5e3, 1.0e3, gr.firdes.WIN_HANN)
        FILT = gr.freq_xlating_fir_filter_ccf(1, coeffs, 0, sample_rate)

        lcm = gru.lcm(sample_rate, reqd_sample_rate)
        intrp = int(lcm // sample_rate)
        decim = int(lcm // reqd_sample_rate)
        RSAMP = blks.rational_resampler_ccc(self, intrp, decim)      

        DEMOD = blks.dqpsk_demod(self,
                                 samples_per_symbol = reqd_sample_rate // symbol_rate,
                                 excess_bw=0.35,
                                 costas_alpha=0.03,
                                 gain_mu=0.05,
                                 mu=0.005,
                                 omega_relative_limit=0.05,
                                 gray_code=options.gray_code,
                                 verbose=options.verbose)

        # FS "$5575f5ff77ff"
        fsm="010101010111010111110101111111110111011111111111"
        CORR = gr.correlate_access_code_bb(fsm, 0)

        OUT = gr.file_sink(gr.sizeof_char, options.output_file)

        self.connect(IN, FILT, RSAMP, DEMOD, CORR, OUT)

def main():
    parser = OptionParser(option_class=eng_option)
    parser.add_option("-d", "--decim", type="int", default=256, help="set decimation")
    parser.add_option("-g", "--gray-code", action="store_true", default=False, help="use gray code")
    parser.add_option("-i", "--input-file", type="string", default="in.dat", help="specify the input file")
    parser.add_option("-o", "--output-file", type="string", default="out.dat", help="specify the output file")
    parser.add_option("-r", "--repeat", action="store_true", default=False, help="repeat input in a loop")
    parser.add_option("-v", "--verbose", action="store_true", default=False, help="dump demodulation data")
    (options, args) = parser.parse_args()

    fg = app_flow_graph(options)
    try:
        fg.run()
    except KeyboardInterrupt:
        pass
 
if __name__ == "__main__":
    main()
