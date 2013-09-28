
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

class trunked_system (object):
    def __init__(self, debug=0, frequency_set=None):
	self.debug = debug
	self.frequency_set = frequency_set
	self.freq_table = {}
	self.stats = {}
	self.stats['tsbks'] = 0
	self.stats['crc'] = 0
	self.tsbk_cache = {}
	self.secondary = {}
	self.adjacent = {}
	self.rfss_syid = 0
	self.rfss_rfid = 0
	self.rfss_stid = 0
	self.rfss_chan = 0
	self.rfss_txchan = 0
	self.ns_syid = 0
	self.ns_wacn = 0
	self.ns_chan = 0

    def to_string(self):
        s = []
        for table in self.freq_table:
            a = self.freq_table[table]['frequency'] / 1000000.0
            b = self.freq_table[table]['step'] / 1000000.0
            c = self.freq_table[table]['offset'] / 1000000.0
            s.append('tbl-id: %x frequency: %f step %f offset %f' % ( table, a,b,c))
            #self.freq_table[table]['frequency'] / 1000000.0, self.freq_table[table]['step'] / 1000000.0, self.freq_table[table]['offset']) / 1000000.0)
        s.append('stats: tsbks %d crc %d' % (self.stats['tsbks'], self.stats['crc']))
        s.append('secondary control channel(s): %s' % ','.join(['%f' % (float(k) / 1000000.0) for k in self.secondary.keys()]))
        for f in self.adjacent:
            s.append('adjacent %f: %s' % (float(f) / 1000000.0, self.adjacent[f]))
        s.append('rf: sysid %x rfid %x stid %x frequency %f uplink %f' % ( self.rfss_syid, self.rfss_rfid, self.rfss_stid, float(self.rfss_chan) / 1000000.0, float(self.rfss_txchan) / 1000000.0))
        s.append('net: sysid %x wacn %x frequency %f' % ( self.ns_syid, self.ns_wacn, float(self.ns_chan) / 1000000.0))
        return '\n'.join(s)

# return frequency in Hz
    def channel_id_to_frequency(self, id):
	table = (id >> 12) & 0xf
	channel = id & 0xfff
	if table not in self.freq_table:
		return None
	return self.freq_table[table]['frequency'] + self.freq_table[table]['step'] * channel

    def channel_id_to_string(self, id):
	table = (id >> 12) & 0xf
	channel = id & 0xfff
	if table not in self.freq_table:
		return "%x-%d" % (table, channel)
	return "%f" % (self.freq_table[table]['frequency'] + self.freq_table[table]['step'] * channel) / 1000000.0

    def decode_mbt_data(self, opcode, header, mbt_data):
	# print "decode_mbt_data: %x %x" %(opcode, mbt_data)
	if opcode == 0x0:  # grp voice channel grant
		ch1  = (mbt_data >> 32) & 0xffff
		ch2  = (mbt_data >> 16) & 0xffff
		ga   = (mbt_data      ) & 0xffff
		if self.debug > 10:
			print "mbt00 voice grant ch1 %x ch2 %x addr 0x%x" %(ch1, ch2, ga)
	elif opcode == 0x3c:  # adjacent status
		rfid = (header >> 24) & 0xff
		stid = (header >> 16) & 0xff
		ch1  = (mbt_data >> 48) & 0xffff
		ch2  = (mbt_data >> 32) & 0xffff
		if self.debug > 10:
			print "mbt3c adjacent rfid %x stid %x ch1 %x ch2 %x" %(rfid, stid, ch1, ch2)
	elif opcode == 0x3b:  # network status
		wacn = (mbt_data >> 44) & 0xfffff
		ch1  = (mbt_data >> 24) & 0xffff
		ch2  = (mbt_data >> 8) & 0xffff
		if self.debug > 10:
			print "mbt3b net stat wacn %x ch1 %x ch2 %x" %(wacn, ch1, ch2)
	elif opcode == 0x3a:  # rfss status
		rfid = (mbt_data >> 56) & 0xff
		stid = (mbt_data >> 48) & 0xff
		ch1  = (mbt_data >> 32) & 0xffff
		ch2  = (mbt_data >> 16) & 0xffff
		if self.debug > 10:
			print "mbt3a rfss stat rfid %x stid %x ch1 %x ch2 %x" %(rfid, stid, ch1, ch2)
	#else:
	#	print "mbt other %x" % opcode

    def decode_tsbk(self, tsbk):
	self.stats['tsbks'] += 1
#	if crc16(tsbk, 12) != 0:
#		self.stats['crc'] += 1
#		return	# crc check failed
	tsbk = tsbk << 16	# for missing crc
	opcode = (tsbk >> 88) & 0x3f
	if self.debug > 10:
		print "TSBK: 0x%02x 0x%024x" % (opcode, tsbk)
	if opcode == 0x02:   # group voice chan grant update
		ch1  = (tsbk >> 64) & 0xffff
		ga1  = (tsbk >> 48) & 0xffff
		ch2  = (tsbk >> 32) & 0xffff
		ga2  = (tsbk >> 16) & 0xffff
		if self.frequency_set:
			frequency = self.channel_id_to_frequency(ch1)
			if frequency:
				self.frequency_set(frequency)
		if self.debug > 10:
			print "tsbk02 grant update: chan %s %d %s %d" %(self.channel_id_to_string(ch1), ga1, self.channel_id_to_string(ch2), ga2)
	elif opcode == 0x16:   # sndcp data ch
		ch1  = (tsbk >> 48) & 0xffff
		ch2  = (tsbk >> 32) & 0xffff
		if self.debug > 10:
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
		self.freq_table[iden]['offset'] = toff * spac * 125
		self.freq_table[iden]['step'] = spac * 125
		self.freq_table[iden]['frequency'] = freq * 5
		if self.debug > 10:
			print "tsbk34 iden vhf/uhf id %d toff %f spac %f freq %f [%s]" % (iden, toff * spac * 0.125 * 1e-3, spac * 0.125, freq * 0.000005, txt[toff_sign])
	elif opcode == 0x3d:   # iden_up
		iden = (tsbk >> 76) & 0xf
		bw   = (tsbk >> 67) & 0x1ff
		toff0 = (tsbk >> 58) & 0x1ff
		spac = (tsbk >> 48) & 0x3ff
		freq = (tsbk >> 16) & 0xffffffff
		toff_sign = (toff0 >> 8) & 1
		toff = toff0 & 0xff
		if toff_sign == 0:
			toff = 0 - toff
		txt = ["mob xmit < recv", "mob xmit > recv"]
		self.freq_table[iden] = {}
		self.freq_table[iden]['offset'] = toff * 250000
		self.freq_table[iden]['step'] = spac * 125
		self.freq_table[iden]['frequency'] = freq * 5
		if self.debug > 10:
			print "tsbk3d iden id %d toff %f spac %f freq %f" % (iden, toff * 0.25, spac * 0.125, freq * 0.000005)
	elif opcode == 0x3a:   # rfss status
		syid = (tsbk >> 56) & 0xfff
		rfid = (tsbk >> 48) & 0xff
		stid = (tsbk >> 40) & 0xff
		chan = (tsbk >> 24) & 0xffff
		f1 = self.channel_id_to_frequency(chan)
		if f1:
			self.rfss_syid = syid
			self.rfss_rfid = rfid
			self.rfss_stid = stid
			self.rfss_chan = f1
			self.rfss_txchan = f1 + self.freq_table[chan >> 12]['offset']
		if self.debug > 10:
			print "tsbk3a rfss status: syid: %x rfid %x stid %d ch1 %x(%s)" %(syid, rfid, stid, chan, self.channel_id_to_string(chan))
	elif opcode == 0x39:   # secondary cc
		rfid = (tsbk >> 72) & 0xff
		stid = (tsbk >> 64) & 0xff
		ch1  = (tsbk >> 48) & 0xffff
		ch2  = (tsbk >> 24) & 0xffff
		f1 = self.channel_id_to_frequency(ch1)
		f2 = self.channel_id_to_frequency(ch2)
		if f1 and f2:
			self.secondary[ f1 ] = 1
			self.secondary[ f2 ] = 1
		if self.debug > 10:
			print "tsbk39 secondary cc: rfid %x stid %d ch1 %x(%s) ch2 %x(%s)" %(rfid, stid, ch1, self.channel_id_to_string(ch1), ch2, self.channel_id_to_string(ch2))
	elif opcode == 0x3b:   # network status
		wacn = (tsbk >> 52) & 0xfffff
		syid = (tsbk >> 40) & 0xfff
		ch1  = (tsbk >> 24) & 0xffff
		f1 = self.channel_id_to_frequency(ch1)
		if f1:
			self.ns_syid = syid
			self.ns_wacn = wacn
			self.ns_chan = f1
		if self.debug > 10:
			print "tsbk3b net stat: wacn %x syid %x ch1 %x(%s)" %(wacn, syid, ch1, self.channel_id_to_string(ch1))
	elif opcode == 0x3c:   # adjacent status
		rfid = (tsbk >> 48) & 0xff
		stid = (tsbk >> 40) & 0xff
		ch1  = (tsbk >> 24) & 0xffff
		f1 = self.channel_id_to_frequency(ch1)
		if f1:
			self.adjacent[f1] = 'rfid: %x stid:%x' % (rfid, stid)
		if self.debug > 10:
			print "tsbk3c adjacent: rfid %x stid %d ch1 %x(%s)" %(rfid, stid, ch1, self.channel_id_to_string(ch1))
	#else:
	#	print "tsbk other %x" % opcode

def main():
	q = 0x3a000012ae01013348704a54
	rc = crc16(q,12)
	print "should be zero: %x" % rc

	q = 0x3a001012ae01013348704a54
	rc = crc16(q,12)
	print "should be nonzero: %x" % rc

	t = trunked_system(debug=255)
	q = 0x3a000012ae0101334870
	t.decode_tsbk(q)
	

if __name__ == '__main__':
	main()
