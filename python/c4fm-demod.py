#!/usr/bin/env python

"""
simple frequency demodulator for captured P25 C4FM signals

input: complex sample captured by USRP
output: real baseband sample at 48 ksps
"""

from gnuradio import gr, gru, blks
import math

class my_graph(gr.flow_graph):

	def __init__(self):
		gr.flow_graph.__init__(self)

		### set these manually for target sample
		input_sample_rate = 500e3	# trunk control channel sample
		# frequency is relative to input sample
		#frequency = 87.5e3			# lower band of trunk control channel sample
		frequency = 100e3			# higher band of trunk control channel sample
		#input_sample_rate = 256e3	# *-256KSS.dat samples
		#frequency = 0				# baseband-complex-256KSS.dat
		#frequency = 30e3			# sample-complex-256KSS.dat
		bandwidth = 12.5e3
		###
		src = gr.file_source(gr.sizeof_gr_complex, "infile")
		symbol_rate = 4800
		output_samples_per_symbol = 10
		output_sample_rate = output_samples_per_symbol * symbol_rate
		lcm = gru.lcm(input_sample_rate, output_sample_rate)
		intrp = int(lcm // input_sample_rate)
		decim = int(lcm // output_sample_rate)
		ddc_coeffs = \
			gr.firdes.low_pass (1.0,
				input_sample_rate,
				bandwidth/2,
				bandwidth/2,
				gr.firdes.WIN_HANN)
		ddc =  gr.freq_xlating_fir_filter_ccf (1,ddc_coeffs,-frequency,input_sample_rate)
		resampler = blks.rational_resampler_ccc(self, intrp, decim)
		qdemod = gr.quadrature_demod_cf(1.0)
		f2c = gr.float_to_complex()
		sink = gr.file_sink(gr.sizeof_float, "demod")

		self.connect(src,ddc,resampler,qdemod,sink)

if __name__ == '__main__':
	try:
		my_graph().run()
	except KeyboardInterrupt:
		pass
