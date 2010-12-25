/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
 * Copyright 2010, KA1RBI 
 */
/*
 * config.h is generated by configure.  It contains the results
 * of probing for features, options etc.  It should be the first
 * file included in your .cc file.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <repeater_p25_frame_assembler.h>
#include <gr_io_signature.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <bch.h>
#include <op25_imbe_frame.h>
#include <op25_p25_frame.h>
#include <p25_framer.h>
#include <repeater_p25_frame_assembler.h>
#include <sys/time.h>
#include <rs.h>

/*
 * Create a new instance of repeater_p25_frame_assembler and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
repeater_p25_frame_assembler_sptr 
repeater_make_p25_frame_assembler (const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr_msg_queue_sptr queue)
{
  return repeater_p25_frame_assembler_sptr (new repeater_p25_frame_assembler (udp_host, port, debug, do_imbe, do_output, do_msgq, queue));
}

static const int MIN_IN = 1;	// mininum number of input streams
static const int MAX_IN = 1;	// maximum number of input streams

/*
 * The private constructor
 */
repeater_p25_frame_assembler::repeater_p25_frame_assembler (const char* udp_host, int port, int debug, bool do_imbe, bool do_output, bool do_msgq, gr_msg_queue_sptr queue)
  : gr_block ("p25_frame_assembler",
		   gr_make_io_signature (MIN_IN, MAX_IN, sizeof (char)),
		   gr_make_io_signature ((do_output) ? 1 : 0, (do_output) ? 1 : 0, (do_output) ? sizeof(char) : 0 )),
	write_bufp(0),
	write_sock(0),
	d_udp_host(udp_host),
	d_port(port),
	d_debug(debug),
	d_do_imbe(do_imbe),
	d_do_output(do_output),
	d_msg_queue(queue),
	symbol_queue(),
	framer(new p25_framer())
{
	if (port > 0)
		init_sock(d_udp_host, d_port);
}

repeater_p25_frame_assembler::~repeater_p25_frame_assembler ()
{
	if (write_sock > 0)
		close(write_sock);
	delete framer;
}

void
repeater_p25_frame_assembler::forecast(int nof_output_items, gr_vector_int &nof_input_items_reqd)
{
   // for do_imbe=false: we output packed bytes (4:1 ratio)
   // for do_imbe=true: input rate= 4800, output rate= 1600 = 32 * 50 (3:1)
   const size_t nof_inputs = nof_input_items_reqd.size();
   int nof_samples_reqd = 4.0 * nof_output_items;
   if (d_do_imbe)
     nof_samples_reqd = 3.0 * nof_output_items;
   std::fill(&nof_input_items_reqd[0], &nof_input_items_reqd[nof_inputs], nof_samples_reqd);
}

int 
repeater_p25_frame_assembler::general_work (int noutput_items,
                               gr_vector_int &ninput_items,
                               gr_vector_const_void_star &input_items,
                               gr_vector_void_star &output_items)
{

  const uint8_t *in = (const uint8_t *) input_items[0];

  for (int i = 0; i < noutput_items; i++){
    if(framer->rx_sym(in[i])) {   // complete frame was detected
		if (d_debug > 0 && framer->duid == 0x00) {
			ProcHDU(framer->frame_body);
		}
		if (d_debug > 10) {
			fprintf (stderr, "NAC 0x%X DUID 0x%X symbols %d BCH errors %d\n", framer->nac, framer->duid, framer->frame_size >> 1, framer->bch_errors);
			switch(framer->duid) {
			case 0x00:	// Header DU
				// see above ProcHDU(framer->frame_body); 
				break;
			case 0x05:	// LDU 1
				ProcLDU1(framer->frame_body);
				break;
			case 0x0a:	// LDU 2
				ProcLDU2(framer->frame_body);
				break;
			case 0x0f:	// LDU 2
				ProcTDU(framer->frame_body);
				break;
			}
		}
		if (d_do_imbe && (framer->duid == 0x5 || framer->duid == 0xa)) {  // if voice - ldu1 or ldu2
			for(size_t i = 0; i < nof_voice_codewords; ++i) {
				voice_codeword cw(voice_codeword_sz);
				uint32_t E0, ET;
				uint32_t u[8];
				char s[128];
				imbe_deinterleave(framer->frame_body, cw, i);
				// recover 88-bit IMBE voice code word
				imbe_header_decode(cw, u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], E0, ET);
				// output one 32-byte msg per 0.020 sec.
				// also, 32*9 = 288 byte pkts (for use via UDP)
				sprintf(s, "%03x %03x %03x %03x %03x %03x %03x %03x\n", u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7]);
				if (d_do_output) {
					for (size_t j=0; j < strlen(s); j++) {
						symbol_queue.push_back(s[j]);
					}
				}
				if (write_sock > 0) {
					memcpy(&write_buf[write_bufp], s, strlen(s));
					write_bufp += strlen(s);
					if (write_bufp >= 288) { // 9 * 32 = 288
						sendto(write_sock, write_buf, 288, 0, (struct sockaddr *)&write_sock_addr, sizeof(write_sock_addr));
						// FIXME check sendto() rc
						write_bufp = 0;
					}
				}
			}
		} // end of imbe/voice
		if (!d_do_imbe) {
			// pack the bits into bytes, MSB first
			size_t obuf_ct = 0;
			uint8_t obuf[P25_VOICE_FRAME_SIZE/2];
			for (uint32_t i = 0; i < framer->frame_size; i += 8) {
				uint8_t b = 
					(framer->frame_body[i+0] << 7) +
					(framer->frame_body[i+1] << 6) +
					(framer->frame_body[i+2] << 5) +
					(framer->frame_body[i+3] << 4) +
					(framer->frame_body[i+4] << 3) +
					(framer->frame_body[i+5] << 2) +
					(framer->frame_body[i+6] << 1) +
					(framer->frame_body[i+7]     );
				obuf[obuf_ct++] = b;
			}
			if (write_sock > 0) {
				sendto(write_sock, obuf, obuf_ct, 0, (struct sockaddr*)&write_sock_addr, sizeof(write_sock_addr));
			}
			if (d_do_output) {
				for (size_t j=0; j < obuf_ct; j++) {
					symbol_queue.push_back(obuf[j]);
				}
			}
		}
    }  // end of complete frame
  }
  int amt_produce = 0;
  amt_produce = noutput_items;
  if (amt_produce > (int)symbol_queue.size())
    amt_produce = symbol_queue.size();
  if (amt_produce > 0) {
    unsigned char *out = (unsigned char *) output_items[0];
    copy(symbol_queue.begin(), symbol_queue.begin() + amt_produce, out);
    symbol_queue.erase(symbol_queue.begin(), symbol_queue.begin() + amt_produce);
  }
  // printf ("work: ninp[0]: %d nout: %d size %d produce: %d surplus %d\n", ninput_items[0], noutput_items, symbol_queue.size(), amt_produce, surplus);
  consume_each(noutput_items);
  // Tell runtime system how many output items we produced.
  return amt_produce;
}

void repeater_p25_frame_assembler::init_sock(const char* udp_host, int udp_port)
{
        memset (&write_sock_addr, 0, sizeof(write_sock_addr));
        write_sock = socket(PF_INET, SOCK_DGRAM, 17);   // UDP socket
        if (write_sock < 0) {
                fprintf(stderr, "op25_imbe_vocoder: socket: %d\n", errno);
                write_sock = 0;
		return;
        }
        if (!inet_aton(udp_host, &write_sock_addr.sin_addr)) {
                fprintf(stderr, "op25_imbe_vocoder: inet_aton: bad IP address\n");
		close(write_sock);
		write_sock = 0;
		return;
	}
        write_sock_addr.sin_family = AF_INET;
        write_sock_addr.sin_port = htons(udp_port);
}
