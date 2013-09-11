
# P25 TSBK and MBT message decoder (C) Copyright 2013 KA1RBI
# 
# This file is part of OP25
# 
# OP25 is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# OP25 is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
# License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with OP25; see the file COPYING. If not, write to the Free
# Software Foundation, Inc., 51 Franklin Street, Boston, MA
# 02110-1301, USA.

def crc16(dat,len):	# slow version
        poly = (1<<12) + (1<<5) + (1<<0)
        crc = 0
        for i in range(len):
                bits = (dat >> (((len-1)-i)*8)) & 0xff
                for j in range(8):
                        bit = (bits >> (7-j)) & 1
                        crc = ((crc << 1) | bit) & 0x1ffff
                        if crc & 0x10000:
                                crc = (crc & 0xffff) ^ poly
        crc = crc ^ 0xffff
        return crc

class tsbk (object):
    def __init__(self, debug=0):
	self.debug = debug
	self.freq_table = {}
	self.stats = {}
	self.stats['tsbks'] = 0
	self.stats['crc'] = 0

    def format_channel_id(self, id):
	table = (id >> 12) & 0xf
	channel = id & 0xfff
	if table not in self.freq_table:
		return "%x-%d" % (table, channel)
	return "%f" % (self.freq_table[table]['frequency'] + self.freq_table[table]['step'] * channel)

    def decode_mbt_data(self, opcode, header, mbt_data):
	# print "decode_mbt_data: %x %x" %(opcode, mbt_data)
	if opcode == 0x0:  # grp voice channel grant
		ch1  = (mbt_data >> 32) & 0xffff
		ch2  = (mbt_data >> 16) & 0xffff
		ga   = (mbt_data      ) & 0xffff
		print "mbt00 voice grant ch1 %x ch2 %x addr 0x%x" %(ch1, ch2, ga)
	elif opcode == 0x3c:  # adjacent status
		rfid = (header >> 24) & 0xff
		stid = (header >> 16) & 0xff
		ch1  = (mbt_data >> 48) & 0xffff
		ch2  = (mbt_data >> 32) & 0xffff
		print "mbt3c adjacent rfid %x stid %x ch1 %x ch2 %x" %(rfid, stid, ch1, ch2)
	elif opcode == 0x3b:  # network status
		wacn = (mbt_data >> 44) & 0xfffff
		ch1  = (mbt_data >> 24) & 0xffff
		ch2  = (mbt_data >> 8) & 0xffff
		print "mbt3b net stat wacn %x ch1 %x ch2 %x" %(wacn, ch1, ch2)
	elif opcode == 0x3a:  # rfss status
		rfid = (mbt_data >> 56) & 0xff
		stid = (mbt_data >> 48) & 0xff
		ch1  = (mbt_data >> 32) & 0xffff
		ch2  = (mbt_data >> 16) & 0xffff
		print "mbt3a rfss stat rfid %x stid %x ch1 %x ch2 %x" %(rfid, stid, ch1, ch2)
	#else:
	#	print "mbt other %x" % opcode

    def decode_tsbk(self, tsbk):
	self.stats['tsbks'] += 1
	if crc16(tsbk, 12) != 0:
		self.stats['crc'] += 1
		return	# crc check failed
	opcode = (tsbk >> 88) & 0x3f
	#print "TSBK: 0x%02x 0x%024x" % (opcode, tsbk)
	if opcode == 0x02:   # group voice chan grant update
		ch1  = (tsbk >> 64) & 0xffff
		ga1  = (tsbk >> 48) & 0xffff
		ch2  = (tsbk >> 32) & 0xffff
		ga2  = (tsbk >> 16) & 0xffff
		print "tsbk02 grant update: chan %s %d %s %d" %(self.format_channel_id(ch1), ga1, self.format_channel_id(ch2), ga2)
	elif opcode == 0x16:   # sndcp data ch
		ch1  = (tsbk >> 48) & 0xffff
		ch2  = (tsbk >> 32) & 0xffff
		print "tsbk16 sndcp data ch: chan %x %x" %(ch1, ch2)
	elif opcode == 0x34:   # iden_up vhf uhf
		iden = (tsbk >> 76) & 0xf
		bwvu = (tsbk >> 72) & 0xf
		toff0 = (tsbk >> 58) & 0x3fff
		spac = (tsbk >> 48) & 0x3ff
		freq = (tsbk >> 16) & 0xffffffff
		toff_sign = (toff0 >> 13) & 1
		toff = toff0 & 0x1fff
		if toff_sign == 0:
			toff = 0 - toff
		txt = ["mob Tx-", "mob Tx+"]
		self.freq_table[iden] = {}
		self.freq_table[iden]['offset'] = float(toff) * float(spac) * 0.125
		self.freq_table[iden]['step'] = float(spac) * 0.000125
		self.freq_table[iden]['frequency'] = float(freq) * 0.000005
		print "tsbk34 iden vhf/uhf id %d toff %f spac %f freq %f [%s]" % (iden, toff * spac * 0.125 * 1e-3, spac * 0.125, freq * 0.000005, txt[toff_sign])
	elif opcode == 0x3d:   # iden_up
		iden = (tsbk >> 76) & 0xf
		bw   = (tsbk >> 67) & 0x1ff
		toff0 = (tsbk >> 58) & 0x1ff
		spac = (tsbk >> 48) & 0x3ff
		freq = (tsbk >> 16) & 0xffffffff
		# toff_sign = (toff0 >> 13) & 1
		# toff = toff0 & 0x1fff
		# if toff_sign == 0:
		# 	toff = 0 - toff
		toff = toff0
		txt = ["mob xmit < recv", "mob xmit > recv"]
		self.freq_table[iden] = {}
		self.freq_table[iden]['offset'] = float(toff) * 0.25
		self.freq_table[iden]['step'] = float(spac) * 0.000125
		self.freq_table[iden]['frequency'] = float(freq) * 0.000005
		print "tsbk3d iden id %d toff %f spac %f freq %f" % (iden, toff * 0.25, spac * 0.125, freq * 0.000005)
	elif opcode == 0x3a:   # rfss status
		syid = (tsbk >> 56) & 0xfff
		rfid = (tsbk >> 48) & 0xff
		stid = (tsbk >> 40) & 0xff
		chan = (tsbk >> 24) & 0xffff
		print "tsbk3a rfss status: syid: %x rfid %x stid %d ch1 %x(%s)" %(syid, rfid, stid, chan, self.format_channel_id(chan))
	elif opcode == 0x39:   # secondary cc
		rfid = (tsbk >> 72) & 0xff
		stid = (tsbk >> 64) & 0xff
		ch1  = (tsbk >> 48) & 0xffff
		ch2  = (tsbk >> 24) & 0xffff
		print "tsbk39 secondary cc: rfid %x stid %d ch1 %x(%s) ch2 %x(%s)" %(rfid, stid, ch1, self.format_channel_id(ch1), ch2, self.format_channel_id(ch2))
	elif opcode == 0x3b:   # network status
		wacn = (tsbk >> 52) & 0xfffff
		syid = (tsbk >> 40) & 0xfff
		ch1  = (tsbk >> 24) & 0xffff
		print "tsbk3b net stat: wacn %x syid %x ch1 %x(%s)" %(wacn, syid, ch1, self.format_channel_id(ch1))
	elif opcode == 0x3c:   # adjacent status
		rfid = (tsbk >> 48) & 0xff
		stid = (tsbk >> 40) & 0xff
		ch1  = (tsbk >> 24) & 0xffff
		print "tsbk3c adjacent: rfid %x stid %d ch1 %x(%s)" %(rfid, stid, ch1, self.format_channel_id(ch1))
	#else:
	#	print "tsbk other %x" % opcode

def main():
	q = 0x3a000012ae01013348704a54
	rc = crc16(q,12)
	print "should be zero: %x" % rc

	q = 0x3a001012ae01013348704a54
	rc = crc16(q,12)
	print "should be nonzero: %x" % rc

	t = tsbk()
	q = 0x3a000012ae01013348704a54
	t.decode_tsbk(q)
	

if __name__ == '__main__':
	# run tests, if this code is invoked as a program
	main()
