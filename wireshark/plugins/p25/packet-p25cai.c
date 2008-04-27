/* packet-p25cai.c
 * Routines for APCO Project 25 Common Air Interface dissection
 * Copyright 2008, Michael Ossmann <mike@ossmann.com>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <epan/packet.h>
#include <epan/prefs.h>

#define FRAME_SYNC_MAGIC 0x5575F5FF77FF

/* function prototypes */
void proto_reg_handoff_p25cai(void);
tvbuff_t* extract_status_symbols(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree);
tvbuff_t* build_hdu_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset);
tvbuff_t* build_tsdu_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset);
tvbuff_t* build_termlc_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset);
void data_deinterleave(tvbuff_t *tvb, guint8 *deinterleaved, int bit_offset);
void trellis_1_2_decode(guint8 *encoded, guint8 *decoded, int offset);
guint8 golay_18_6_8_decode(guint32 codeword);
guint16 golay_24_12_8_decode(guint32 codeword);
void rs_24_12_13_decode(guint8 *codeword, guint8 *decoded);
void rs_36_20_17_decode(guint8 *codeword, guint8 *decoded);
int find_min(guint8 list[], int len);
int count_bits(unsigned int n);

/* Initialize the protocol and registered fields */
static int proto_p25cai = -1;
static int hf_p25cai_fs = -1;
static int hf_p25cai_nid = -1;
static int hf_p25cai_nac = -1;
static int hf_p25cai_duid = -1;
static int hf_p25cai_hdu = -1;
static int hf_p25cai_tsbk = -1;
static int hf_p25cai_pdu = -1;
static int hf_p25cai_ldu1 = -1;
static int hf_p25cai_ldu2 = -1;
static int hf_p25cai_mi = -1;
static int hf_p25cai_mfid = -1;
static int hf_p25cai_algid = -1;
static int hf_p25cai_kid = -1;
static int hf_p25cai_tgid = -1;
static int hf_p25cai_ss_parent = -1;
static int hf_p25cai_ss = -1;
static int hf_p25cai_lc = -1;
static int hf_p25cai_lcf = -1;
static int hf_p25cai_lbf = -1;
static int hf_p25cai_ptbf = -1;
static int hf_p25cai_opcode = -1;
static int hf_p25cai_args = -1;
static int hf_p25cai_crc = -1;

/* Field values */
static const value_string data_unit_ids[] = {
	{ 0x0, "Header Data Unit" },
	{ 0x3, "Terminator without Link Control" },
	{ 0x5, "Logical Link Data Unit 1" },
	{ 0x7, "Trunking Signaling Data Unit" },
	{ 0xA, "Logical Link Data Unit 2" },
	{ 0xC, "Packet Data Unit" },
	{ 0xF, "Terminator with Link Control" },
	{ 0, NULL }
};

/* Initialize the subtree pointers */
static gint ett_p25cai = -1;
static gint ett_ss = -1;
static gint ett_nid = -1;
static gint ett_du = -1;

/* Code to actually dissect the packets */
static int
dissect_p25cai(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{

/* Set up structures needed to add the protocol subtree and manage it */
	proto_item *ti, *nid_item, *du_item;
	proto_tree *p25cai_tree, *nid_tree, *du_tree;
	int offset;
	tvbuff_t *extracted_tvb, *du_tvb;
	guint8 duid, last_block;

/*  If this doesn't look like a P25 CAI frame, give up and return 0 so that
 *  perhaps another dissector can take over.
 */

	/* Check that there's enough data */
	if (tvb_length(tvb) < 14) /* P25 CAI smallest packet size */
		return 0;

	/* Check for correct Frame Sync value
	 *
	 * This is commented out for now so that we can deal with symbol errors.
	 * We could use it if we have error correction working before frames get
	 * passed to wireshark.  Perhaps a more generally useful alternative would
	 * look for a minimum number of matching bits rather than requiring a 100%
	 * match.
	 *
	 * guint64 fs;
	 * fs = tvb_get_ntoh64(tvb, 0) >> 16;
	 * if (fs != FRAME_SYNC_MAGIC)
	 * 	return 0;
	 */

/* Make entries in Protocol column and Info column on summary display */
	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "P25 CAI");

	/* Clear the Info column first just in case of duid fetching failure. */
	if (check_col(pinfo->cinfo, COL_INFO))
		col_clear(pinfo->cinfo, COL_INFO);

	duid = tvb_get_guint8(tvb, 7) & 0xF;

	if (check_col(pinfo->cinfo, COL_INFO))
		col_set_str(pinfo->cinfo, COL_INFO, val_to_str(duid, data_unit_ids, "Unknown Data Unit (0x%02x)"));

	/* see if we are being asked for details */
	if (tree) {

/* create display subtree for the protocol */
		ti = proto_tree_add_item(tree, proto_p25cai, tvb, 0, -1, FALSE);
		p25cai_tree = proto_item_add_subtree(ti, ett_p25cai);
		offset = 0;

		/* top level P25 CAI tree */
		proto_tree_add_item(p25cai_tree, hf_p25cai_fs, tvb, offset, 6, FALSE);
		offset += 6;

		/* Extract Status Symbols */
		extracted_tvb = extract_status_symbols(tvb, pinfo, p25cai_tree);

		/* process extracted_tvb from here on */

		nid_item = proto_tree_add_item(p25cai_tree, hf_p25cai_nid, extracted_tvb, offset, 8, FALSE);

		/* NID subtree */
		nid_tree = proto_item_add_subtree(nid_item, ett_nid);
		proto_tree_add_item(nid_tree, hf_p25cai_nac, extracted_tvb, offset, 2, FALSE);
		proto_tree_add_item(nid_tree, hf_p25cai_duid, extracted_tvb, offset, 2, FALSE);
		offset += 8;

		switch (duid) {
		/* Header Data Unit */
		case 0x0:
			du_tvb = build_hdu_tvb(extracted_tvb, pinfo, offset);
			offset = 0;
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_hdu, du_tvb, offset, -1, FALSE);
			du_tree = proto_item_add_subtree(du_item, ett_du);
			proto_tree_add_item(du_tree, hf_p25cai_mi, du_tvb, offset, 9, FALSE);
			offset += 9;
			proto_tree_add_item(du_tree, hf_p25cai_mfid, du_tvb, offset, 1, FALSE);
			offset += 1;
			proto_tree_add_item(du_tree, hf_p25cai_algid, du_tvb, offset, 1, FALSE);
			offset += 1;
			proto_tree_add_item(du_tree, hf_p25cai_kid, du_tvb, offset, 2, FALSE);
			offset += 2;
			proto_tree_add_item(du_tree, hf_p25cai_tgid, du_tvb, offset, 2, FALSE);
			offset += 2;
			break;
		/* Terminator Data Unit without Link Control */
		case 0x3:
			/* nothing left to decode */
			break;

		/* TODO: much more dissection below */

		/* Logical Link Data Unit 1 */
		case 0x5:
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_ldu1, extracted_tvb, offset, -1, FALSE);
			break;
		/* Trunking Signaling Data Unit */
		case 0x7:
			du_tvb = build_tsdu_tvb(extracted_tvb, pinfo, offset);
			offset = 0;
			last_block = 0;
			while (last_block == 0) {
				last_block = tvb_get_guint8(du_tvb, offset) >> 7;
				du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_tsbk, du_tvb, offset, 12, FALSE);
				du_tree = proto_item_add_subtree(du_item, ett_du);
				proto_tree_add_item(du_tree, hf_p25cai_lbf, du_tvb, offset, 1, FALSE);
				proto_tree_add_item(du_tree, hf_p25cai_ptbf, du_tvb, offset, 1, FALSE);
				proto_tree_add_item(du_tree, hf_p25cai_opcode, du_tvb, offset, 1, FALSE);
				offset += 1;
				proto_tree_add_item(du_tree, hf_p25cai_mfid, du_tvb, offset, 1, FALSE);
				offset += 1;
				proto_tree_add_item(du_tree, hf_p25cai_args, du_tvb, offset, 8, FALSE);
				offset += 8;
				/* TODO: more items */
				proto_tree_add_item(du_tree, hf_p25cai_crc, du_tvb, offset, 2, FALSE);
				offset += 2;
			}
			break;
		/* Logical Link Data Unit 2 */
		case 0xA:
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_ldu2, extracted_tvb, offset, -1, FALSE);
			break;
		/* Packet Data Unit */
		case 0xC:
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_pdu, extracted_tvb, offset, -1, FALSE);
			break;
		/* Terminator Data Unit with Link Control */
		case 0xF:
			du_tvb = build_termlc_tvb(extracted_tvb, pinfo, offset);
			offset = 0;
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_lc, du_tvb, offset, -1, FALSE);
			du_tree = proto_item_add_subtree(du_item, ett_du);
			proto_tree_add_item(du_tree, hf_p25cai_lcf, du_tvb, offset, 1, FALSE);
			offset += 1;
			/* TODO: Decode Link Control according to Link Control Format. */
			break;
		/* Unknown Data Unit */
		default:
			/* don't know how to decode any more */
			break;
		}

	}

/* Return the amount of data this dissector was able to dissect */
	return tvb_length(tvb);
}

/* Extract status symbols, display them, and return frame without status symbols */
tvbuff_t*
extract_status_symbols(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	proto_item *ss_parent_item;
	proto_tree *ss_tree;
	int i, j, raw_length, extracted_length;
	guint8 *extracted_buffer;
	tvbuff_t *extracted_tvb;

	raw_length = tvb_length(tvb);
	extracted_length = raw_length - (raw_length / 36);

	/* Create buffer to become new tvb. */
	extracted_buffer = (guint8*)ep_alloc0(extracted_length);

	/* Create status symbol subtree. */
	ss_parent_item = proto_tree_add_item(tree, hf_p25cai_ss_parent, tvb, 0, -1, FALSE);
	ss_tree = proto_item_add_subtree(ss_parent_item, ett_ss);

	/* Go through frame one dibit at a time. */
	for (i = 0, j = 0; i < raw_length * 4; i++) {
		if (i % 36 == 35) {
			/* After every 35 dibits is a status symbol. */
			proto_tree_add_item(ss_tree, hf_p25cai_ss, tvb, i/4, 1, FALSE);
		} else {
			/* Extract frame bits from between status symbols. */
			/* I'm sure there is a more efficient way to do this. */
			extracted_buffer[j/4] |= tvb_get_bits8(tvb, i * 2, 2) << (6 - (j % 4) * 2);
			j++;
		}
	}

	/* Setup a new tvb buffer with the extracted data. */
	extracted_tvb = tvb_new_real_data(extracted_buffer, extracted_length, extracted_length);
	tvb_set_child_real_data_tvbuff(tvb, extracted_tvb);
	add_new_data_source(pinfo, extracted_tvb, "P25 CAI");

	return extracted_tvb;
}

/* Build Header Data Unit tvb. */
tvbuff_t*
build_hdu_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset)
{
	guint8 *hdu_buffer, *rs_codeword;
	guint8 rs_code_byte, high_byte, low_byte;
	guint32 golay_codeword;
	int i, j;
	tvbuff_t *hdu_tvb;

	DISSECTOR_ASSERT(tvb_length_remaining(tvb, offset) >= 81);

	rs_codeword = (guint8*)ep_alloc0(27);
	hdu_buffer = (guint8*)ep_alloc0(15);

	/* Each 18 bits is a Golay codeword. */
	for (i = offset * 8, j = 0; j < 216; i += 18, j += 6) {

		/* Take 18 bits from the tvb, adjusting for byte boundaries. */
		golay_codeword = (tvb_get_ntohl(tvb, i / 8) >> (14 - i % 8)) & 0x3FFFF;
		rs_code_byte = golay_18_6_8_decode(golay_codeword) << 2;

		/* Stuff high bits into one byte of the new buffer. */
		high_byte = rs_code_byte >> (j % 8);
		rs_codeword[j / 8] |= high_byte;

		/* Stuff low bits into the next unless beyond end of buffer. */
		if (j < 210) {
			low_byte = rs_code_byte << (8 - j % 8);
			rs_codeword[j / 8 + 1] |= low_byte;
		}
	}

	rs_36_20_17_decode(rs_codeword, hdu_buffer);

	/* Setup a new tvb buffer with the decoded data. */
	hdu_tvb = tvb_new_real_data(hdu_buffer, 15, 15);
	tvb_set_child_real_data_tvbuff(tvb, hdu_tvb);
	add_new_data_source(pinfo, hdu_tvb, "data units");

	return hdu_tvb;
}

/* Build Trunking Signaling Block tvb. */
tvbuff_t*
build_tsdu_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset)
{
	guint8 *tsdu_buffer, *trellis_buffer;
	guint8 last_block;
	int tsdu_offset, tvb_bit_offset;
	tvbuff_t *tsdu_tvb;

	/* From here on, our tvb offset may not fall on bit boundaries, so we track
	 * it by bits instead of bytes.
	 */
	tvb_bit_offset = offset * 8;
	tsdu_offset = 0;
	last_block = 0;

	tsdu_buffer = (guint8*)ep_alloc0(36); /* 12 times maximum number of TSBKs (3) */

	while (last_block == 0) {
		DISSECTOR_ASSERT(tvb_length_remaining(tvb, tvb_bit_offset / 8) >= 25);
		trellis_buffer = (guint8*)ep_alloc0(25);
		data_deinterleave(tvb, trellis_buffer, tvb_bit_offset);
		trellis_1_2_decode(trellis_buffer, tsdu_buffer, tsdu_offset);
		tvb_bit_offset += 196;
		last_block = tsdu_buffer[tsdu_offset] >> 7;
		tsdu_offset += 12;
	}

	/* Setup a new tvb buffer with the decoded data. */
	tsdu_tvb = tvb_new_real_data(tsdu_buffer, tsdu_offset, tsdu_offset);
	tvb_set_child_real_data_tvbuff(tvb, tsdu_tvb);
	add_new_data_source(pinfo, tsdu_tvb, "data units");

	return tsdu_tvb;
}

/* Build Terminator Data Unit with Link Control tvb. */
/* TODO: This one is completely untested. */
tvbuff_t*
build_termlc_tvb(tvbuff_t *tvb, packet_info *pinfo, int offset)
{
	guint8 *termlc_buffer, *rs_codeword;
	guint16 rs_code_chunk;
	guint8 high_byte, low_byte;
	guint32 golay_codeword;
	int i, j;
	tvbuff_t *termlc_tvb;

	DISSECTOR_ASSERT(tvb_length_remaining(tvb, offset) >= 36);

	rs_codeword = (guint8*)ep_alloc0(18);
	termlc_buffer = (guint8*)ep_alloc0(9);

	/* Each 24 bits is a Golay codeword. */
	for (i = offset * 8, j = 0; j < 144; i += 24, j += 12) {

		/* Take 24 bits from the tvb, adjusting for byte boundaries. */
		golay_codeword = (tvb_get_ntohl(tvb, i / 8) >> (8 - i % 8)) & 0xFFFFFF;
		rs_code_chunk = golay_24_12_8_decode(golay_codeword) << 4;

		/* Stuff high bits into one byte of the new buffer. */
		high_byte = rs_code_chunk >> (j % 8);
		rs_codeword[j / 8] |= high_byte;

		/* Stuff low bits into the next. */
		low_byte = rs_code_chunk << (8 - j % 8);
		rs_codeword[j / 8 + 1] |= low_byte;
	}

	rs_24_12_13_decode(rs_codeword, termlc_buffer);

	/* Setup a new tvb buffer with the decoded data. */
	termlc_tvb = tvb_new_real_data(termlc_buffer, 15, 15);
	tvb_set_child_real_data_tvbuff(tvb, termlc_tvb);
	add_new_data_source(pinfo, termlc_tvb, "data units");

	return termlc_tvb;
}

/* Deinterleave data block.  Assumes output buffer is already zeroed. */
void
data_deinterleave(tvbuff_t *tvb, guint8 *deinterleaved, int bit_offset)
{
	int d, i, j, t;
	int steps[] = {0, 52, 100, 148};

	/* step through input nibbles to copy to output */
	for (i = bit_offset, d = 0; i < 45 + bit_offset; i += 4) {
		for (j = 0; j < 4; j++, d += 4) {
			/* t = tvb bit index
			 * d = deinterleaved bit index
			 */
			t = i + steps[j];
			deinterleaved[d / 8] |= ((tvb_get_guint8(tvb, t / 8) << (t % 8)) & 0xF0) >> (d % 8);
		}
	}
	t = bit_offset + 48;
	deinterleaved[24] |= ((tvb_get_guint8(tvb, t / 8) << (t % 8)) & 0xF0);
}

/* 1/2 rate trellis decoder.  Assumes output buffer is already zeroed. */
void
trellis_1_2_decode(guint8 *encoded, guint8 *decoded, int offset)
{
	int i, j, k;
	int state = 0;
	guint8 codeword;

	/* Hamming distance */
	guint8 hd[4];

	/* state transition table, including constellation to dibit pair mapping */
	guint8 next_words[4][4] = {
		{0x2, 0xC, 0x1, 0xF},
		{0xE, 0x0, 0xD, 0x3},
		{0x9, 0x7, 0xA, 0x4},
		{0x5, 0xB, 0x6, 0x8}
	};

	/* step through 4 bit codewords in input */
	for (i = 0; i < 196; i += 4) {
		codeword = (encoded[i / 8] >> (4 - (i % 8))) & 0xF;
		for (k = 0; k < 4; k++)
			hd[k] = 0;
		/* try each codeword in a row of the state transition table */
		for (j = 0; j < 4; j++) {
			/* find Hamming distance for candidate */
			hd[j] = count_bits(codeword ^ next_words[state][j]);
		}
		/* find the dibit that matches the most codeword bits (minimum Hamming distance) */
		state = find_min(hd, 4);
		/* error if minimum can't be found */
		DISSECTOR_ASSERT(state != -1);
		/* It also might be nice to report a condition where the minimum is
		 * non-zero, i.e. an error has been corrected.  It probably shouldn't
		 * be a permanent failure, though.
		 *
		 * DISSECTOR_ASSERT(hd[state] == 0);
		 */

		/* append dibit onto output buffer */
		if (i < 192)
			decoded[(i / 16) + offset] |= state << (6 - (i / 2) % 8);
	}
}

/* Error correction decoders
 *
 * TODO:  For now these are fake.  They pull out the original bits without
 * actually doing any error correction or detection.
 */

/* fake (18,6,8) shortened Golay decoder, no error correction */
/* TODO: make less fake */
guint8
golay_18_6_8_decode(guint32 codeword)
{
	return (codeword >> 12) & 0x3F;
}

/* fake (24,12,8) extended Golay decoder, no error correction */
/* TODO: make less fake */
guint16
golay_24_12_8_decode(guint32 codeword)
{
	return (codeword >> 12) & 0xFFF;
}

/* fake (24,12,13) Reed-Solomon decoder, no error correction */
/* TODO: make less fake */
void
rs_24_12_13_decode(guint8 *codeword, guint8 *decoded)
{
	int i;

	/* Just grab the first 9 bytes (12 sets of six bits) */
	for (i = 0; i < 9; i++)
		decoded[i] = codeword[i];
}

/* fake (36,20,17) Reed-Solomon decoder, no error correction */
/* TODO: make less fake */
void
rs_36_20_17_decode(guint8 *codeword, guint8 *decoded)
{
	int i;

	/* Just grab the first 15 bytes (20 sets of six bits) */
	for (i = 0; i < 15; i++)
		decoded[i] = codeword[i];
}

/* Register the protocol with Wireshark */
void
proto_register_p25cai(void)
{

/* Setup list of fields */
	static hf_register_info hf[] = {
		{ &hf_p25cai_fs,
			{ "Frame Synchronization", "p25cai.fs",
			FT_BYTES, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_nid,
			{ "Network ID Codeword", "p25cai.nid",
			FT_UINT64, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_nac,
			{ "Network Access Code", "p25cai.nac",
			FT_UINT16, BASE_HEX, NULL, 0xFFF0,
			NULL, HFILL }
		},
		{ &hf_p25cai_duid,
			{ "Data Unit ID", "p25cai.duid",
			FT_UINT16, BASE_HEX, VALS(data_unit_ids), 0x000F,
			NULL, HFILL }
		},
		{ &hf_p25cai_hdu,
			{ "Header Data Unit", "p25cai.hdu",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_tsbk,
			{ "Trunking Signaling Block", "p25cai.tsbk",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_pdu,
			{ "Packet Data Unit", "p25cai.pdu",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_ldu1,
			{ "Logical Link Data Unit 1", "p25cai.ldu1",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_ldu2,
			{ "Logical Link Data Unit 2", "p25cai.ldu2",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_mi,
			{ "Message Indicator", "p25cai.mi",
			FT_BYTES, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_mfid,
			{ "Manufacturer's ID", "p25cai.mfid",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_algid,
			{ "Algorithm ID", "p25cai.algid",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_kid,
			{ "Key ID", "p25cai.kid",
			FT_UINT16, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_tgid,
			{ "Talk-group ID", "p25cai.tgid",
			FT_UINT16, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_ss_parent,
			{ "Status Symbols", "p25cai.ss_parent",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_ss,
			{ "Status Symbol", "p25cai.ss",
			FT_UINT8, BASE_HEX, NULL, 0x3,
			NULL, HFILL }
		},
		{ &hf_p25cai_lc,
			{ "Link Control", "p25cai.lc",
			FT_NONE, BASE_NONE, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_lcf,
			{ "Link Control Format", "p25cai.lcf",
			FT_UINT8, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_lbf,
			{ "Last Block Flag", "p25cai.lbf",
			FT_BOOLEAN, BASE_NONE, NULL, 0x80,
			NULL, HFILL }
		},
		{ &hf_p25cai_ptbf,
			{ "Protected Trunking Block Flag", "p25cai.ptbf",
			FT_BOOLEAN, BASE_NONE, NULL, 0x40,
			NULL, HFILL }
		},
		{ &hf_p25cai_opcode,
			{ "Opcode", "p25cai.opcode",
			FT_UINT8, BASE_HEX, NULL, 0x3F,
			NULL, HFILL }
		},
		{ &hf_p25cai_args,
			{ "Arguments", "p25cai.args",
			FT_UINT64, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		},
		{ &hf_p25cai_crc,
			{ "CRC", "p25cai.crc",
			FT_UINT16, BASE_HEX, NULL, 0x0,
			NULL, HFILL }
		}
	};

/* Setup protocol subtree arrays */
	static gint *ett[] = {
		&ett_p25cai,
		&ett_ss,
		&ett_nid,
		&ett_du
	};

/* Register the protocol name and description */
	proto_p25cai = proto_register_protocol(
		"APCO Project 25 Common Air Interface",		/* full name */
		"P25 CAI",	/* short name */
		"p25cai"	/* abbreviation (e.g. for filters) */
		);

/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_p25cai, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

}

void
proto_reg_handoff_p25cai(void)
{
	static gboolean inited = FALSE;

	if (!inited) {

		dissector_handle_t p25cai_handle;

/*  Use new_create_dissector_handle() to indicate that dissect_p25cai()
 *  returns the number of bytes it dissected (or 0 if it thinks the packet
 *  does not belong to APCO Project 25 Common Air Interface).
 */
		p25cai_handle = new_create_dissector_handle(dissect_p25cai,
			proto_p25cai);
		/* FIXME: Temporarily using a reserved ethertype!  This is a
		 * development shortcut that should be corrected later.  Perhaps we
		 * will apply for a libpcap data link type or use other mechanisms in
		 * the future.  Also, we can always use "decode as" in Wireshark.
		 */
		dissector_add("ethertype", 0xFFFF, p25cai_handle);

		inited = TRUE;
	}
}


/* Utility functions */

/* count the number of 1 bits in an int */
int
count_bits(unsigned int n)
{
    int i = 0;
    for (i = 0; n != 0; i++)
        n &= n - 1;
    return i;
}

/* return the index of the lowest value in a list */
int
find_min(guint8 list[], int len)
{
	int min = list[0];	
	int index = 0;	
	int unique = 1;	
	int i;

	for (i = 1; i < len; i++) {
		if (list[i] < min) {
			min = list[i];
			index = i;
			unique = 1;
		} else if (list[i] == min) {
			unique = 0;
		}
	}
	/* return -1 if a minimum can't be found */
	if (!unique)
		return -1;

	return index;
}
