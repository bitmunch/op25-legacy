#!/usr/bin/python
import struct, sys
from gnuradio.eng_option import eng_option
from optparse import OptionParser

"""
prototype P25 frame decoder

input: short frequency demodulated signal capture including at least one frame (output of c4fm_demod.py)
output: decoded frames

This horrifying mess is a prototype for stuff that will most likely end up as gnuradio signal processing blocks.
"""

parser = OptionParser(option_class=eng_option)
parser.add_option("-i", "--input-file", type="string", default="demod.dat", help="specify the input file")
parser.add_option("-s", "--samples-per-symbol", type="int", default=10, help="samples per symbol of the input file")
(options, args) = parser.parse_args()

# frame synchronization header (in form most useful for correlation)
frame_sync = [1, 1, 1, 1, 1, -1, 1, 1, -1, -1, 1, 1, -1, -1, -1, -1, 1, -1, 1, -1, -1, -1, -1, -1]
minimum_span = options.samples_per_symbol // 2 # minimum number of adjacent correlations required to convince us
# maximum number of symbols to look for in a frame  (Is this correct?)
max_frame_length = 864
symbol_rate = 4800
data = open(options.input_file).read()
input_samples = struct.unpack('f1'*(len(data)/4), data)
sync_samples = [] # subset of input samples synchronized with respect to frame sync
symbols = [] # recovered symbols (including frame sync)

# collect input sample statistics for normalization
total_value = 0.0
total_deviation = 0.0
count = 0
for sample in input_samples:
	total_value += sample
	total_deviation += abs(sample)
mean_value = total_value / len(input_samples)
mean_deviation = total_deviation / len(input_samples)

# return average (mean) of a list of values
def average(list):
	total = 0.0
	for num in list: total += num
	return total / len(list)

# return integer represented by sequence of dibits
def dibits_to_integer(dibits):
	integer = 0
	for dibit in dibits:
		integer = integer << 2
		integer += dibit
	return integer

# return integer represented by sequence of tribits
def tribits_to_integer(tribits):
	integer = 0
	for tribit in tribits:
		integer = integer << 3
		integer += tribit
	return integer

# extract a block of symbols from in between status symbols
# return extracted block, list of status symbols, and number of symbols consumed
def extract_block(symbols, length, index):
	maximum_raw_length = length + 2 + (length // 36)
	block = symbols[index:index + maximum_raw_length]
	status_symbols = []
	# adjust position of first status symbol according to starting index of input
	start = 36 - (index % 36) - 1
	# find the indices of all the status symbols within the block
	indices = range(start,maximum_raw_length,36)
	# start from the end so we don't pop out from under ourselves
	indices.reverse()
	for i in indices:
		status_symbols.append(block.pop(i))
	status_symbols.reverse()
	return block[:length], status_symbols, length + len(status_symbols)

# de-interleave a sequence of 98 symbols (for data or control channel frame)
def data_deinterleave(input):
	output = []
	for i in range(0,23,2):
		for j in (0, 26, 50, 74):
			output.extend(input[i+j:i+j+2])
	output.extend(input[24:26])
	return output

# 1/2 rate trellis decode a sequence of symbols
# TODO: Use soft symbols instead of hard symbols.
def trellis_1_2_decode(input):
	if len(input) != 98:
		raise NameError('wrong input length for trellis_1_2_decode')
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
		codeword = dibits_to_integer(input[i:i+2])
		similarity = [0, 0, 0, 0]
		# compare codeword against each of four candidates for the current state
		for candidate in range(4):
			# increment similarity result for each bit in codeword that matches candidate
			for bit in range(4):
				if ((~codeword ^ next_words[state][candidate]) & (1 << bit)) > 0:
					similarity[candidate] += 1
		# find the dibit that matches all four codeword bits
		if similarity.count(4) == 1:
			state = similarity.index(4)
		# otherwise find the dibit that matches three codeword bits
		elif similarity.count(3) == 1:
			state = similarity.index(3)
			print "corrected single trellis error"
		else:
			raise NameError('ambiguous trellis decoding error')
		output.append(state)
	return output[:48]

# 3/4 rate trellis decode a sequence of symbols
# TODO: Use soft symbols instead of hard symbols.
def trellis_3_4_decode(input):
	if len(input) != 98:
		raise NameError('wrong input length for trellis_1_2_decode')
	output = []
	# state transition table, including constellation to dibit pair mapping
	next_words = (
		(0x2, 0xD, 0xE, 0x1, 0x7, 0x8, 0xB, 0x4),
		(0xE, 0x1, 0x7, 0x8, 0xB, 0x4, 0x2, 0xD),
		(0xA, 0x5, 0x6, 0x9, 0xF, 0x0, 0x3, 0xC),
		(0x6, 0x9, 0xF, 0x0, 0x3, 0xC, 0xA, 0x5),
		(0xF, 0x0, 0x3, 0xC, 0xA, 0x5, 0x6, 0x9),
		(0x3, 0xC, 0xA, 0x5, 0x6, 0x9, 0xF, 0x0),
		(0x7, 0x8, 0xB, 0x4, 0x2, 0xD, 0xE, 0x1),
		(0xB, 0x4, 0x2, 0xD, 0xE, 0x1, 0x7, 0x8))
	state = 0
	# cycle through 2 symbol codewords in input
	for i in range(0,len(input),2):
		codeword = dibits_to_integer(input[i:i+2])
		similarity = [0, 0, 0, 0, 0, 0, 0, 0]
		# compare codeword against each of eight candidates for the current state
		for candidate in range(8):
			# increment similarity result for each bit in codeword that matches candidate
			for bit in range(4):
				if ((~codeword ^ next_words[state][candidate]) & (1 << bit)) > 0:
					similarity[candidate] += 1
		# find the dibit that matches all four codeword bits
		if similarity.count(4) == 1:
			state = similarity.index(4)
		# otherwise find the dibit that matches three codeword bits
		elif similarity.count(3) == 1:
			state = similarity.index(3)
			print "corrected single trellis error"
		else:
			raise NameError('ambiguous trellis decoding error')
		output.append(state)
	return tribits_to_integer(output[:48])

# fake (64,16,23) BCH decoder, no error correction
# spec sometimes refers to this as (63,16,23) plus a parity bit
# TODO: make less fake
def bch_64_16_23_decode(input):
	return input[:8]

# fake (18,6,8) shortened Golay decoder, no error correction
# TODO: make less fake
def golay_18_6_8_decode(input):
	output = []
	for i in range(0,len(input),9):
		codeword = input[i:i+9]
		output.extend(codeword[:3])
	return output

# fake (24,12,8) extended Golay decoder, no error correction
# TODO: make less fake
def golay_24_12_8_decode(input):
	output = []
	for i in range(0,len(input),12):
		codeword = input[i:i+12]
		output.extend(codeword[:6])
	return output

# fake (10,6,3) shortened Hamming decoder, no error correction
# TODO: make less fake
def hamming_10_6_3_decode(input):
	output = []
	for i in range(0,len(input),5):
		codeword = input[i:i+5]
		output.extend(codeword[:3])
	return output

# fake (24,12,13) Reed-Solomon decoder, no error correction
# TODO: make less fake
def rs_24_12_13_decode(input):
	return input[:36]

# fake (24,16,9) Reed-Solomon decoder, no error correction
# TODO: make less fake
def rs_24_16_9_decode(input):
	return input[:48]

# fake (36,20,17) Reed-Solomon decoder, no error correction
# TODO: make less fake
def rs_36_20_17_decode(input):
	return input[:60]

# fake (16_8_5) shortened cyclic decoder, no error correction
# TODO: make less fake
def cyclic_16_8_5_decode(input):
	output = []
	for i in range(0,len(input),8):
		codeword = input[i:i+8]
		output.extend(codeword[:4])
	return output

# fake IMBE frame decoder
# TODO: make less fake
#
# each IMBE frame encodes 20 ms of voice in 88 bits
# words u_0, u_1, . . . u_7 may be encrypted
# 56 coding bits added for total of 144 bits
# 48 most important bits protected by (23,12,7) Golay code
# (u_0, u_1, u_2, u_3 12 bits each)
# TODO: Golay decoding
# next 33 important bits protected by (15,11,3 Hamming code
# (u_4, u_5, u_6, 11 bits each)
# TODO: Hamming decoding
# 7 least important bits unprotected
# (u_7 7 bits)
# words u_1, u_2, . . . u_6 are further xored by PN based on u_0
# TODO: de-randomization
# TODO: de-interleaving
def imbe_decode(input):
	print "Raw IMBE frame: 0x%036x" % (dibits_to_integer(input))

# correlate (multiply-accumulate) frame sync
def correlate(start):
	first = 0
	last = 0
	interesting = 0
	correlation_threshold = len(frame_sync) * mean_deviation
	for i in range(start,len(input_samples) - len(frame_sync) * options.samples_per_symbol):
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
	if last:
		# return center point of several adjacent correlations
		return first + ((last - first) // 2)
	else:
		return 0

# downsample to symbol rate (with integrate and dump)
def downsample(start):
	sync_samples = []
	# grab samples for symbols starting with frame sync
	for i in range(start, start + (max_frame_length * options.samples_per_symbol), options.samples_per_symbol):
		# "integrate and dump"
		# Add up several (samples_per_symbol) adjacent samples to create a single
		# (downsampled) sample as specified by the P25 CAI standard.
		total = 0.0
		start = i - (options.samples_per_symbol/2)
		end = i + 1 + (options.samples_per_symbol/2)
		for sample in input_samples[start:end]: total += sample
		sync_samples.append(total)
		#print i, sync_samples[-1]
	return sync_samples

def hard_decision(samples):
	symbols = []
	# determine symbol thresholds
	highs = []
	lows = []
	# Check out the frame sync samples since we are certain what symbols
	# each one ought to be. The frame sync only uses half (the highest and
	# lowest) of the four frequency deviations.
	for i in range(len(frame_sync)):
		if frame_sync[i] == 1:
			highs.append(samples[i])
		else:
			lows.append(samples[i])
	high = average(highs)
	low = average(lows)
	# Use these averages to establish thresholds between ranges of frequency deviations.
	step = (high - low) / 6
	low_threshold = low + step
	middle_threshold = low + (3 * step)
	high_threshold = low + (5 * step)
	
	# assign each sample to a symbol (hard decision)
	for sample in samples:
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
	return symbols

# We have to do a little decoding in order to know how long the frame is.
# Plus, we may want to use soft values for error correction decoding.  Then we
# can dump the complete frame to wireshark.
#
# basic map:
# frame_sync = symbols[0:24]
# network_identifier = symbols[24:57] (including status symbol at symbols[35])
# unknown_blocks = symbols[57:?] (including status symbols after every 35 symbols)

def decode_frame(symbols):
	consumed = 0
	status_symbols = []

	print "Frame Sync: 0x%012x" % dibits_to_integer(symbols[0:24])
	consumed += 24

	nid_symbols, block_status_symbols, block_consumed = extract_block(symbols, 32, consumed)
	status_symbols.extend(block_status_symbols)
	consumed += block_consumed
	network_id = bch_64_16_23_decode(nid_symbols)
	network_access_code = dibits_to_integer(network_id[:6])
	data_unit_id = dibits_to_integer(network_id[6:])
	
	print "NID codeword: 0x%016x" % (dibits_to_integer(nid_symbols))
	print "Network Access Code: 0x%03x, Data Unit ID: 0x%01x" % (network_access_code, data_unit_id)
	
	if data_unit_id == 7:
		print "Found a trunking control channel packet in single block format"
		# contains one to four Trunking Signaling Blocks (TSBK)
		last_block_flag = 0
		while last_block_flag < 1:
			print "Found a Trunking Signaling Block"
			tsbk_symbols, block_status_symbols, block_consumed = extract_block(symbols, 98, consumed)
			status_symbols.extend(block_status_symbols)
			consumed += block_consumed
			# spec diagram indicates status symbol at symbols[158] (single TSBK), but I've only found nulls
			# TODO: maybe there is a status symbol after the nulls
			tsbk = trellis_1_2_decode(data_deinterleave(tsbk_symbols))
			# TODO: verify with CRC
			# TODO: see 102.AABC-B for further decoding
			print "TSBK: 0x%024x" % (dibits_to_integer(tsbk))
			last_block_flag = tsbk[0] >> 1
	elif data_unit_id == 12:
		print "Found a Packet Data Unit"
		# need to decode header to find out what the rest of the frame looks like
		header_symbols, block_status_symbols, block_consumed = extract_block(symbols, 98, consumed)
		status_symbols.extend(block_status_symbols)
		consumed += block_consumed
		header = trellis_1_2_decode(data_deinterleave(header_symbols))
		# A/N indicates whether or not confirmation is desired, 1 bit
		an = header[0] % 2
		# I/O indicates direction of message (1 = outbound, 0 = inbound), 1 bit
		io = header[1] >> 1
		# Format of PDU, 5 bits
		format = dibits_to_integer(header[1:4]) & 0x1F
		# TODO: header crc_ccitt error detection
		if format == 0x16:
			print "PDU is a Confirmed Data Packet"
			# block length in octets
			block_size = 16
			# Service Access Point (SAP) to which the message is directed, 6 bits
			sap_id = dibits_to_integer(header[4:8]) & 0x3F
			# Manufacturer's ID (MFID) 8 bits
			manufacturers_id = dibits_to_integer(header_data_unit[8:12])
			# Logical Link ID, 24 bits
			logical_link_id = dibits_to_integer(header_data_unit[12:24])
			# Full Message Flag (FMF) (1 for first try, 0 for retries), 1 bit
			fmf = header[24] >> 1
			# Blocks to Follow, 7 bits
			blocks_to_follow = dibits_to_integer(header[24:28]) & 0x7F
			# Pad Octet Count, 5 bits
			pad_octet_count = dibits_to_integer(header[29:32]) & 0x1F
			# Synchronize (Syn), 1 bit
			syn = header[32] >> 1
			# Sequence Number (N(S)), 3 bits
			sequence_number = dibits_to_integer(header[32:34]) & 0x7
			# Fragment Sequence Number Field (FSNF), 4 bits
			fragment_sequence_number = dibits_to_integer(header[34:36])
			# Data Header Offset, 6 bits
			data_header_offset = dibits_to_integer(header[36:40]) & 0x3F
			# Header CRC, 16 bits
			header_crc = dibits_to_integer(header[40:48])
			# TODO: verify header CRC
			user_data = []
			for i in range(blocks_to_follow):
				data_block_symbols, block_status_symbols, block_consumed = extract_block(symbols, 98, consumed)
				status_symbols.extend(block_status_symbols)
				consumed += block_consumed
				data_block = trellis_3_4_decode(data_deinterleave(data_block_symbols))
				# data_block is 144 bits
				# Data Block Serial Number, 7 bits
				data_block_serial_number = (data_block >> 128) & 0xFE00
				# CRC-9, 9 bits
				data_block_crc = (data_block >> 128) & 0x01FF
				#TODO: verify data block CRC
				user_data.extend(data_block & 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF)
			packet_crc = user_data & 0xFFFFFFFF
			user_data = user_data >> 32
			#TODO: verify packet CRC
		elif format == 0x3:
			print "PDU is a Response Packet"
			# block length in octets
			block_size = 12
			response_class = header[4]
			response_type = dibits_to_integer(header[5:7]) & 0xE
			response_status = dibits_to_integer(header[6:8]) & 0x3
			# Manufacturer's ID (MFID) 8 bits
			manufacturers_id = dibits_to_integer(header_data_unit[8:12])
			# Logical Link ID, 24 bits
			logical_link_id = dibits_to_integer(header_data_unit[12:24])
			# X, 1 bit
			x = header[24] >> 1
			# Blocks to Follow, 7 bits
			blocks_to_follow = dibits_to_integer(header[24:28]) & 0x7F
			# Source Logical Link ID, 24 bits
			source_logical_link_id = dibits_to_integer(header_data_unit[28:40])
			# Header CRC, 16 bits
			header_crc = dibits_to_integer(header[40:48])
			# TODO: verify header CRC
			response_data = []
			for i in range(blocks_to_follow):
				data_block_symbols, block_status_symbols, block_consumed = extract_block(symbols, 98, consumed)
				status_symbols.extend(block_status_symbols)
				consumed += block_consumed
				data_block = trellis_1_2_decode(data_deinterleave(data_block_symbols))
				response_data.extend(data_block[:32])
			# Not absolutely certain that this CRC location is correct.  Spec unclear if more than one data block.
			packet_crc = response_data[-16:]
			response_data = response_data[:-16]
			#TODO: verify packet CRC
		elif format == 0x15:
			print "PDU is an Unconfirmed Data Packet"
			# could be a multi-block control channel packet (check SAP)
			# block length in octets
			block_size = 12
			# Service Access Point (SAP) to which the message is directed, 6 bits
			sap_id = dibits_to_integer(header[4:8]) & 0x3F
			# Manufacturer's ID (MFID) 8 bits
			manufacturers_id = dibits_to_integer(header_data_unit[8:12])
			# Logical Link ID, 24 bits
			logical_link_id = dibits_to_integer(header_data_unit[12:24])
			# Blocks to Follow, 7 bits
			blocks_to_follow = dibits_to_integer(header[24:28]) & 0x7F
			# Pad Octet Count, 5 bits
			pad_octet_count = dibits_to_integer(header[29:32]) & 0x1F
			# Data Header Offset, 6 bits
			data_header_offset = dibits_to_integer(header[36:40]) & 0x3F
			# Header CRC, 16 bits
			header_crc = dibits_to_integer(header[40:48])
			# TODO: verify header CRC
			user_data = []
			for i in range(blocks_to_follow):
				data_block_symbols, block_status_symbols, block_consumed = extract_block(symbols, 98, consumed)
				status_symbols.extend(block_status_symbols)
				consumed += block_consumed
				data_block = trellis_1_2_decode(data_deinterleave(data_block_symbols))
				user_data.extend(data_block[:32])
			packet_crc = user_data[-16:]
			user_data = user_data[:-16]
			#TODO: verify packet CRC
		elif format == 0x17:
			print "PDU is an Alternate Multiple Block Trunking (MBT) Control Packet"
			# see 102.AABC-B for more details
			# TODO: decode
		else:
			print "PDU has unknown format"
			# how long is frame?
			# block_size = ?
			# error condition?
		# TODO: Enhanced Addressing Format (variation of Confirmed or Unconfirmed)
	elif data_unit_id == 0:
		print "Found a Header Data Unit"
		# Header Data Unit aka Header Word
		# header used prior to superframe
		hdu_symbols, block_status_symbols, block_consumed = extract_block(symbols, 324, consumed)
		status_symbols.extend(block_status_symbols)
		consumed += block_consumed
		rs_codeword = golay_18_6_8_decode(hdu_symbols)
		header_data_unit = rs_36_20_17_decode(rs_codeword)
		# Message Indicator (MI) 72 bits
		message_indicator = header_data_unit[:36]
		print "Message Indicator: 0x%018x" % (dibits_to_integer(message_indicator))
		# Manufacturer's ID (MFID) 8 bits
		manufacturers_id = header_data_unit[36:40]
		print "Manufacturer's ID: 0x%02x" % (dibits_to_integer(manufacturers_id))
		if dibits_to_integer(manufacturers_id) > 0:
			print "Non-standard Manufacturer's ID"
		# Algorithm ID (ALGID) 8 bits
		algorithm_id = header_data_unit[40:44]
		print "Algorithm ID: 0x%02x" % (dibits_to_integer(algorithm_id))
		# Key ID (KID) 16 bits
		key_id = header_data_unit[44:52]
		print "Key ID: 0x%04x" % (dibits_to_integer(key_id))
		# Talk-group ID (TGID) 16 bits
		talk_group_id = header_data_unit[52:60]
		print "Talk Group ID: 0x%04x" % (dibits_to_integer(talk_group_id))
	elif data_unit_id == 3:
		print "Found a Terminator Data Unit without subsequent Link Control"
		# terminator used after superframe
		# may follow LDU1 or LDU2
	elif data_unit_id == 15:
		print "Found a Terminator Data Unit with subsequent Link Control"
		# terminator used after superframe
		# may follow LDU1 or LDU2
		terminator_symbols = symbols[57:216]
		terminator_symbols, block_status_symbols, block_consumed = extract_block(symbols, 144, consumed)
		status_symbols.extend(block_status_symbols)
		consumed += block_consumed
		golay_codewords = terminator_symbols[:144]
		rs_codeword = golay_24_12_8_decode(golay_codewords)
		link_control = rs_24_12_13_decode(rs_codeword)
		print "Link Control: 0x%018x" % (dibits_to_integer(link_control))
	elif data_unit_id == 5:
		print "Found a Logical Link Data Unit 1 (LDU1)"
		# contains voice frames 1 through 9 of a superframe
		ldu_symbols, block_status_symbols, block_consumed = extract_block(symbols, 784, consumed)
		status_symbols.extend(block_status_symbols)
		consumed += block_consumed
		imbe_symbols = []
		imbe_symbols.append(ldu_symbols[:72])
		imbe_symbols.append(ldu_symbols[72:144])
		imbe_symbols.append(ldu_symbols[164:236])
		imbe_symbols.append(ldu_symbols[256:328])
		imbe_symbols.append(ldu_symbols[348:420])
		imbe_symbols.append(ldu_symbols[440:512])
		imbe_symbols.append(ldu_symbols[532:604])
		imbe_symbols.append(ldu_symbols[624:696])
		imbe_symbols.append(ldu_symbols[712:784])
		for frame in imbe_symbols:
			imbe_decode(frame)
		link_control_symbols = ldu_symbols[144:164] + ldu_symbols[236:256] + ldu_symbols[328:348] + ldu_symbols[420:440] + ldu_symbols[512:532] + ldu_symbols[604:624]
		low_speed_data_symbols = ldu_symbols[696:712]
		link_control_rs_codeword = hamming_10_6_3_decode(link_control_symbols)
		link_control = rs_24_12_13_decode(link_control_rs_codeword)
		print "Link Control: 0x%018x" % (dibits_to_integer(link_control))
		link_control_format = link_control[:4]
		print "Link Control Format: 0x%02x" % (dibits_to_integer(link_control_format))
		# rest of link control frame is 64 bits of fields specified by LCF
		# likeliy to include TGID, Source ID, Destination ID, Emergency indicator, MFID
		low_speed_data = cyclic_16_8_5_decode(low_speed_data_symbols)
		print "Low Speed Data: 0x%04x" % (dibits_to_integer(low_speed_data))
		# spec says Low Speed Data = 32 bits data + 32 bits parity
		# huh? Is the other half in LDU2?
	elif data_unit_id == 10:
		# contains voice frames (codewords) 10 through 18 of a superframe
		print "Found a Logical Link Data Unit 2 (LDU2)"
		ldu_symbols, block_status_symbols, block_consumed = extract_block(symbols, 784, consumed)
		status_symbols.extend(block_status_symbols)
		consumed += block_consumed
		imbe_symbols = []
		imbe_symbols.append(ldu_symbols[:72])
		imbe_symbols.append(ldu_symbols[72:144])
		imbe_symbols.append(ldu_symbols[164:236])
		imbe_symbols.append(ldu_symbols[256:328])
		imbe_symbols.append(ldu_symbols[348:420])
		imbe_symbols.append(ldu_symbols[440:512])
		imbe_symbols.append(ldu_symbols[532:604])
		imbe_symbols.append(ldu_symbols[624:696])
		imbe_symbols.append(ldu_symbols[712:784])
		for frame in imbe_symbols:
			imbe_decode(frame)
		encryption_sync_symbols = ldu_symbols[144:164] + ldu_symbols[236:256] + ldu_symbols[328:348] + ldu_symbols[420:440] + ldu_symbols[512:532] + ldu_symbols[604:624]
		low_speed_data_symbols = ldu_symbols[696:712]
		encryption_sync_rs_codeword = hamming_10_6_3_decode(encryption_sync_symbols)
		encryption_sync = rs_24_16_9_decode(encryption_sync_rs_codeword)
		print "Encryption Sync Word: 0x%024x" % (dibits_to_integer(encryption_sync))
		# Message Indicator (MI) 72 bits
		message_indicator = encryption_sync[:36]
		print "Message Indicator: 0x%018x" % (dibits_to_integer(message_indicator))
		# Algorithm ID (ALGID) 8 bits
		algorithm_id = encryption_sync[36:40]
		print "Algorithm ID: 0x%02x" % (dibits_to_integer(algorithm_id))
		# Key ID (KID) 16 bits
		key_id = encryption_sync[40:48]
		print "Key ID: 0x%04x" % (dibits_to_integer(key_id))
		low_speed_data = cyclic_16_8_5_decode(low_speed_data_symbols)
		print "Low Speed Data: 0x%04x" % (dibits_to_integer(low_speed_data))
	else:
		print "unknown Data Unit ID"
	# TODO: maybe extract status symbol after nulls
	print "Status Symbols:",
	for ss in status_symbols:
		print "0x%01x" % ss,
	print
	return consumed

# main loop
#
# This is a first pass at carving this mess up into repeatable chunks.  Bear with me.
correlation_index = 0
for i in range(100):
	frame_start = correlate(correlation_index)
	if frame_start > 0:
		time = float(frame_start) / (symbol_rate * options.samples_per_symbol)
		print "Frame detected at %.3f seconds." % time
		sync_samples = downsample(frame_start)
		symbols = hard_decision(sync_samples)
		consumed = decode_frame(symbols)
		correlation_index = frame_start + ((consumed - 1) * options.samples_per_symbol)
		print
	else:
		print "Reached end of input without correlation."
		break
