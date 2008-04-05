#!/usr/bin/python
import struct, sys
from gnuradio.eng_option import eng_option
from optparse import OptionParser

"""
prototype P25 frame decoder

input: short frequency demodulated signal capture including at least one frame (output of c4fm_demod.py)
output: symbols of a single frame (plus some excess)
"""

parser = OptionParser(option_class=eng_option)
parser.add_option("-i", "--input-file", type="string", default="demod.dat", help="specify the input file")
parser.add_option("-s", "--samples-per-symbol", type="int", default=10, help="samples per symbol of the input file")
(options, args) = parser.parse_args()

# frame synchronization header (in form most useful for correlation)
frame_sync = [1, 1, 1, 1, 1, -1, 1, 1, -1, -1, 1, 1, -1, -1, -1, -1, 1, -1, 1, -1, -1, -1, -1, -1]
minimum_span = 5 # minimum number of adjacent correlations required to convince us
data = open(options.input_file).read()
input_samples = struct.unpack('f1'*(len(data)/4), data)
sync_samples = [] # subset of input samples synchronized with respect to frame sync
symbols = [] # recovered symbols (including frame sync)

# return average (mean) of a list of values
def average(list):
	total = 0.0
	for num in list: total += num
	return total / len(list)

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

# hey, it's a start. . .
print symbols[:500]
