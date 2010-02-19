#!/usr/bin/env python
from gnuradio import gr, gru, blks2
from gnuradio.eng_option import eng_option
from optparse import OptionParser

"""
simple frequency demodulator for captured P25 C4FM signals

input: complex sample captured by USRP
output: real baseband sample at 48 ksps
"""

class my_top_block(gr.top_block):
	def __init__(self, options):
		gr.top_block.__init__(self)

		bandwidth = 12.5e3
		symbol_rate = 4800

		output_sample_rate = options.samples_per_symbol * symbol_rate
		lcm = gru.lcm(options.sample_rate, output_sample_rate)
		intrp = int(lcm // options.sample_rate)
		decim = int(lcm // output_sample_rate)

		if options.input_file == '-':
			src = gr.file_descriptor_source(gr.sizeof_gr_complex, 0)
		else:
			src = gr.file_source(gr.sizeof_gr_complex, options.input_file)

		ddc_coeffs = \
			gr.firdes.low_pass (1.0,
				options.sample_rate,
				bandwidth/2,
				bandwidth/2,
				gr.firdes.WIN_HANN)
		ddc =  gr.freq_xlating_fir_filter_ccf (1,ddc_coeffs,-options.frequency,options.sample_rate)
		resampler = blks2.rational_resampler_ccc(intrp, decim, None, None)
		qdemod = gr.quadrature_demod_cf(1.0)

		if options.output_file == '-':
			sink = gr.file_descriptor_sink(gr.sizeof_float, 1)
		else:
			sink = gr.file_sink(gr.sizeof_float, options.output_file)

		if options.invert:
			inverter = gr.multiply_const_ff(-1.0)
			self.connect(src,ddc,resampler,qdemod,inverter,sink)
		else:
			self.connect(src,ddc,resampler,qdemod,sink)

def main():
	parser = OptionParser(option_class=eng_option)
	parser.add_option("-i", "--input-file", type="string", default="in.dat", help="specify the input file")
	parser.add_option("-o", "--output-file", type="string", default="demod.dat", help="specify the output file")
	parser.add_option("-r", "--sample-rate", type="eng_float", default=500e3, help="sample rate of input file", metavar="Hz")
	parser.add_option("-f", "--frequency", type="eng_float", default=0, help="frequency of signal to demodulate", metavar="Hz")
	parser.add_option("-s", "--samples-per-symbol", type="int", default=10, help="samples per symbol in output file")
	parser.add_option("-a", "--invert", action="store_true", dest="invert", default=False, help="invert output")

	(options, args) = parser.parse_args()

	tb = my_top_block(options)
	try:
		tb.run()
	except KeyboardInterrupt:
		pass

if __name__ == "__main__":
	main()
