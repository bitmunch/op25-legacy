#!/usr/bin/env python
import sys
import math
from gnuradio import gr, gru, audio, eng_notation, blks2, optfir, fsk4, repeater
from gnuradio import op25_imbe
from gnuradio.eng_option import eng_option
from optparse import OptionParser

class my_top_block(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        parser = OptionParser(option_class=eng_option)
        parser.add_option("-a", "--audio-input", type="string", default="")
        parser.add_option("-A", "--audio-output", type="string", default="")
        parser.add_option("-f", "--factor", type="eng_float", default=1)
        parser.add_option("-s", "--sample-rate", type="int", default=48000, help="input sample rate")
        parser.add_option("-S", "--stretch", type="int", default=0, help="flex amt")
        parser.add_option("-y", "--symbol-rate", type="int", default=4800, help="input symbol rate")
        parser.add_option("-v", "--verbose", action="store_true", default=False, help="dump demodulation data")
        (options, args) = parser.parse_args()
 
        sample_rate = options.sample_rate
        symbol_rate = options.symbol_rate

        IN = audio.source(sample_rate, options.audio_input)
        OUT = audio.sink(8000, options.audio_output)

        symbol_decim = 1
        symbol_coeffs = gr.firdes.root_raised_cosine(1.0,	# gain
                                          sample_rate ,	# sampling rate
                                          symbol_rate,  # symbol rate
                                          0.2,     	# width of trans. band
                                          500) 		# filter type 
        SYMBOL_FILTER = gr.fir_filter_fff (symbol_decim, symbol_coeffs)
        AMP = gr.multiply_const_ff(options.factor)
        msgq = gr.msg_queue(2)
        FSK4 = fsk4.demod_ff(msgq, sample_rate, symbol_rate)
        DECODE = repeater.imbe_decode_fb()
        IMBE = op25_imbe.vocoder(False,                 # 0=Decode,True=Encode
                                  options.verbose,      # Verbose flag
                                  options.stretch,      # flex amount
                                  "",                   # udp ip address
                                  0,                    # udp port
                                  False)                # dump raw u vectors

        CVT = gr.short_to_float()
        AMP2 = gr.multiply_const_ff(1.0 / 32767.0)

        self.connect(IN, AMP, SYMBOL_FILTER, FSK4, DECODE, IMBE, CVT, AMP2, OUT)

if __name__ == "__main__":
    try:
        my_top_block().run()
    except KeyboardInterrupt:
        tb.stop()
