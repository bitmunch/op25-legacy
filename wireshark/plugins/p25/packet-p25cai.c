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

/* Forward declaration we need below */
void proto_reg_handoff_p25cai(void);

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
static int hf_p25cai_termlc = -1;
static int hf_p25cai_mi = -1;
static int hf_p25cai_mfid = -1;
static int hf_p25cai_algid = -1;
static int hf_p25cai_kid = -1;
static int hf_p25cai_tgid = -1;

/* Field values */
static const value_string data_unit_ids[] = {
	{ 0x0, "Header Data Unit" },
	{ 0x3, "Terminator without Link Control" },
	{ 0x5, "Logical Link Data Unit 1" },
	{ 0x7, "Single Block Format Trunking Control Channel Packet" },
	{ 0xA, "Logical Link Data Unit 2" },
	{ 0xC, "Packet Data Unit" },
	{ 0xF, "Terminator with Link Control" },
	{ 0, NULL }
};

/* Initialize the subtree pointers */
static gint ett_p25cai = -1;
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
	guint8 duid;

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

		/* TODO: status symbol extraction */
		/* FIXME: Much of the dissection below is broken without status symbol extraction. */

		nid_item = proto_tree_add_item(p25cai_tree, hf_p25cai_nid, tvb, offset, 8, FALSE);

		/* NID subtree */
		nid_tree = proto_item_add_subtree(nid_item, ett_nid);
		proto_tree_add_item(nid_tree, hf_p25cai_nac, tvb, offset, 2, FALSE);
		proto_tree_add_item(nid_tree, hf_p25cai_duid, tvb, offset, 2, FALSE);
		offset += 8;

		switch (duid) {
		/* Header Data Unit */
		case 0x0:
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_hdu, tvb, offset, -1, FALSE);
			du_tree = proto_item_add_subtree(du_item, ett_du);
			proto_tree_add_item(du_tree, hf_p25cai_mi, tvb, offset, 9, FALSE);
			offset += 9;
			proto_tree_add_item(du_tree, hf_p25cai_mfid, tvb, offset, 1, FALSE);
			offset += 1;
			proto_tree_add_item(du_tree, hf_p25cai_algid, tvb, offset, 1, FALSE);
			offset += 1;
			proto_tree_add_item(du_tree, hf_p25cai_kid, tvb, offset, 2, FALSE);
			offset += 2;
			proto_tree_add_item(du_tree, hf_p25cai_tgid, tvb, offset, 2, FALSE);
			offset += 2;
			break;
		/* Terminator Data Unit without Link Control */
		case 0x3:
			/* nothing left to decode */
			break;

		/* TODO: much more dissection below */

		/* Logical Link Data Unit 1 */
		case 0x5:
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_ldu1, tvb, offset, -1, FALSE);
			break;
		/* Trunking control channel packet in single block format */
		case 0x7:
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_tsbk, tvb, offset, -1, FALSE);
			break;
		/* Logical Link Data Unit 2 */
		case 0xA:
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_ldu2, tvb, offset, -1, FALSE);
			break;
		/* Packet Data Unit */
		case 0xC:
			du_item = proto_tree_add_item(p25cai_tree, hf_p25cai_pdu, tvb, offset, -1, FALSE);
			break;
		/* Terminator Data Unit with Link Control */
		case 0xF:
			proto_tree_add_item(p25cai_tree, hf_p25cai_termlc, tvb, offset, -1, FALSE);
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
		{ &hf_p25cai_termlc,
			{ "Terminator Data Unit with Link Control", "p25cai.termlc",
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
		}
	};

/* Setup protocol subtree arrays */
	static gint *ett[] = {
		&ett_p25cai,
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
