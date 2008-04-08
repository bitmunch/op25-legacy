#!/usr/bin/python
import struct, sys
from gnuradio.eng_option import eng_option
from optparse import OptionParser

"""
prototype P25 frame decoder

input: short frequency demodulated signal capture including at least one frame (output of c4fm_demod.py)
output: symbols of a single frame (plus some excess)

This horrifying mess is a prototype for stuff that will most likely end up as gnuradio signal processing blocks.
"""

parser = OptionParser(option_class=eng_option)
parser.add_option("-i", "--input-file", type="string", default="demod.dat", help="specify the input file")
parser.add_option("-s", "--samples-per-symbol", type="int", default=10, help="samples per symbol of the input file")
(options, args) = parser.parse_args()

# frame synchronization header (in form most useful for correlation)
frame_sync = [1, 1, 1, 1, 1, -1, 1, 1, -1, -1, 1, 1, -1, -1, -1, -1, 1, -1, 1, -1, -1, -1, -1, -1]
minimum_span = options.samples_per_symbol // 2 # minimum number of adjacent correlations required to convince us
data = open(options.input_file).read()
input_samples = struct.unpack('f1'*(len(data)/4), data)
sync_samples = [] # subset of input samples synchronized with respect to frame sync
symbols = [] # recovered symbols (including frame sync)

# return average (mean) of a list of values
def average(list):
	total = 0.0
	for num in list: total += num
	return total / len(list)

# return integer represented by sequence of symbols (dibits)
def symbols_to_integer(symbols):
	integer = 0
	for symbol in symbols:
		integer = integer << 2
		integer += symbol
	return integer

# de-interleave a sequence of 98 symbols
def deinterleave(input):
	output = []
	for i in range(0,23,2):
		for j in (0, 26, 50, 74):
			output.extend(input[i+j:i+j+2])
	output.extend(input[24:26])
	return output

# 1/2 rate trellis decode a sequence of symbols
# TODO: Use soft symbols instead of hard symbols.
def trellis_1_2_decode(input):
	output = []
	# state transition table, including constellation to dibit pair mapping
	next_words = (
		(0x2, 0xC, 0x1, 0xF),
		(0xE, 0x0, 0xD, 0x3),
		(0x9, 0x7, 0xA, 0x4),
		(0x5, 0xB, 0x6, 0x8))
	state = 0
	# cycle through 2 symbol codewords in input
	for i in range(0,len(input),2):
		codeword = symbols_to_integer(input[i:i+2])
		#print "codeword", codeword
		similarity = [0, 0, 0, 0]
		# compare codeword against each of four candidates for the current state
		for candidate in range(4):
			# increment similarity result for each bit in codeword that matches candidate
			for bit in range(4):
				if ((~codeword ^ next_words[state][candidate]) & (1 << bit)) > 0:
					similarity[candidate] += 1
		#print "candidates", next_words[state]
		#print "similarity", similarity
		# find the dibit that matches all four codeword bits
		# TODO: exception handling: at least try the next best match
		state = similarity.index(4)
		output.append(state)
		#print "new state", state
		#print output
	return output

# collect input sample statistics for normalization
total_value = 0.0
total_deviation = 0.0
count = 0
for sample in input_samples:
	total_value += sample
	total_deviation += abs(sample)
mean_value = total_value / len(input_samples)
mean_deviation = total_deviation / len(input_samples)

# correlate (multiply-accumulate) frame sync
first = 0
last = 0
interesting = 0
correlation_threshold = len(frame_sync) * mean_deviation
for i in range(len(input_samples) - len(frame_sync) * options.samples_per_symbol):
	correlation = 0
	for j in range(len(frame_sync)):
		correlation += frame_sync[j] * (input_samples[i + j * options.samples_per_symbol] - mean_value)
	#print i, correlation
	if interesting:
		if correlation < correlation_threshold:
			if i >= (first + minimum_span):
				last = i
				break
			else:
				first = 0
				interesting = 0
	elif correlation >= correlation_threshold:
		first = i
		interesting = 1

# downsample to symbol rate (with integrate and dump)
if last:
	# use center point of several adjacent correlations
	center = first + ((last - first) // 2)
	# grab samples for symbol thereafter
	for i in range(center, len(input_samples), options.samples_per_symbol):
		# "integrate and dump"
		# Add up several (samples_per_symbol) adjacent samples to create a single
		# (downsampled) sample as specified by the P25 CAI standard.
		total = 0.0
		start = i - (options.samples_per_symbol/2)
		end = i + 1 + (options.samples_per_symbol/2)
		for sample in input_samples[start:end]: total += sample
		sync_samples.append(total)
		#print i, sync_samples[-1]

# determine symbol thresholds
highs = []
lows = []
# Check out the frame sync samples since we are certain what symbols
# each one ought to be. The frame sync only uses half (the highest and
# lowest) of the four frequency deviations.
for i in range(len(frame_sync)):
	if frame_sync[i] == 1:
		highs.append(sync_samples[i])
	else:
		lows.append(sync_samples[i])
high = average(highs)
low = average(lows)
# Use these averages to establish thresholds between ranges of frequency deviations.
step = (high - low) / 6
low_threshold = low + step
middle_threshold = low + (3 * step)
high_threshold = low + (5 * step)

# assign each sample to a symbol
for sample in sync_samples:
	if sample < low_threshold:
		# dibit 0b11
		symbols.append(3)
	elif sample < middle_threshold:
		# dibit 0b10
		symbols.append(2)
	elif sample < high_threshold:
		# dibit 0b00
		symbols.append(0)
	else:
		# dibit 0b01
		symbols.append(1)

# We have to do a little decoding in order to know how long the frame is.
# Plus, we may want to use soft values for error correction decoding.  Then we
# can dump the complete frame to wireshark.
#
# basic map:
# frame_sync = symbols[0:24]
# network_identifier = symbols[24:57] (including status symbol at symbols[35])
# unknown_blocks = symbols[57:?] (including status symbols after every 35 symbols)

print "frame sync: 0x%012x" % symbols_to_integer(symbols[0:24])
# extract Network Identifier from in between status symbols
nid_symbols = symbols[24:35] + symbols[36:57]

# TODO: This is terrible.  Until we have a BCH decoder, just trust that the first 16 bits are correct
address = symbols_to_integer(nid_symbols[:6])
data_unit_id = symbols_to_integer(nid_symbols[6:8])

print "NID codeword: 0x%016x" % (symbols_to_integer(nid_symbols))
print "address: 0x%03x, Data Unit ID: 0x%01x, first Status Symbol: 0x%01x" % (address, data_unit_id, symbols[35])

if data_unit_id == 7:
	# this is a trunking control channel packet in the single block format aka a Trunking Signaling Block (TSBK)
	print "found a TSBK"
	# spec indicates status symbol at symbols[158] (if no more TSBKs), but I've only found nulls
	#print "Status Symbols: 0x%01x, 0x%01x, 0x%01x, 0x%01x" % (symbols[71], symbols[107], symbols[143], symbols[158])
	print "Status Symbols: 0x%01x, 0x%01x, 0x%01x" % (symbols[71], symbols[107], symbols[143])
	tsbk_symbols = symbols[57:71] + symbols[72:107] + symbols[108:143] + symbols[144:158]
	#print "TSBK symbols: 0x%049x" % (symbols_to_integer(tsbk_symbols))
	#print "TSBK deinterleaved: 0x%049x" % (symbols_to_integer(deinterleave(tsbk_symbols)))
	tsbk_decoded = trellis_1_2_decode(deinterleave(tsbk_symbols))
	# TODO: verify with CRC
	print "TSBK decoded: 0x%025x" % (symbols_to_integer(tsbk_decoded))
	last_block_flag = tsbk_decoded[0] >> 1
	if not last_block_flag:
		print "found another TSBK"
		# TODO: rework to handle up to a total of four TSBKs
elif data_unit_id == 12:
	print "found a Packet Data Unit"
	# could be a multi-block control channel packet
	# could be a confirmed or unconfirmed data packet
	# TODO: decode
elif data_unit_id == 0:
	print "found a Header Data Unit"
	# header used prior to superframe
	# TODO: decode
	# total of 396 symbols (including frame sync)
elif data_unit_id == 3:
	print "found a Terminator without subsequent Link Control"
	# terminator used after superframe
	# TODO: decode
elif data_unit_id == 15:
	print "found a Terminator with subsequent Link Control"
	# terminator used after superframe
	# TODO: decode
elif data_unit_id == 5:
	print "found a Logical Link Data Unit 1"
	# contains voice frames (codewords) 1 through 9 of a superframe
	# TODO: decode
elif data_unit_id == 10:
	print "found a Logical Link Data Unit 2"
	# contains voice frames (codewords) 10 through 18 of a superframe
	# TODO: decode
else:
	print "unknown Data Unit ID"
