/* -*- C++ -*- */

/*
 * Copyright 2008 Steve Glass
 * 
 * This file is part of OP25.
 * 
 * OP25 is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * OP25 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with OP25; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Boston, MA
 * 02110-1301, USA.
 */

#include <hdu.h>
#include <iomanip>
#include <itpp/comm/egolay.h>
#include <itpp/comm/reedsolomon.h>
#include <sstream>
#include <swab.h>
#include <value_string.h>

using namespace std;

hdu::hdu(const_bit_queue& frame_body) :
   abstract_data_unit(frame_body)
{
}

hdu::~hdu()
{
}

string
hdu::duid_str() const
{
   return string("HDU");
}

std::string
hdu::snapshot() const
{
   ostringstream os;

   // DUID
   os << "(dp0" << endl;
   os << "S'duid'" << endl;
   os << "p1" << endl;
   os << "S'" << duid_str() << "'"  << endl;

   // NAC
   os << "p2" << endl;
   os << "sS'nac'" << endl;
   os << "p3" << endl;
   os << "S'" << nac_str() << "'" << endl;

#if 0
   // MI
   os << "p4" << endl;
   os << "sS'mi'" << endl;
   os << "p5" << endl;
   os << "S'" << mi_str() << "'" << endl;
#endif

   os << "p4" << endl;
   os << "s." << endl;

#if 0
   // Source
   os << "S'source'" << endl;
   os << "p5"       << endl;
   os << "S''" << endl;
   os << "p6"       << endl;
   // Dest
   os << "S'dest'" << endl;
   os << "p7"       << endl;
   os << "S''" << endl;
   os << "p8"       << endl;
   // NID
   os << "S'nid'" << endl;
   os << "p9"     << endl;
   os << "S''" << endl;
   os << "p10"    << endl;
   // MFID
   os << "S'mfid'" << endl;
   os << "p11"     << endl;
   os << "S'" << mfid_str() << "'" << endl;
   os << "p12"       << endl;
   // ALGID
   os << "p3"      << endl;
   os << "S'algid'" << endl;
   os << "p4"      << endl;
   os << "sS'" << algid_str() << "'" << endl;
   // KID
   os << "S'kid'" << endl;
   os << "p15"    << endl;
   os << "S''" << endl;
   os << "p16"    << endl;

   // TGID
   os << "S'tgid'" << endl;
   os << "p19"     << endl;
   os << "S''" << endl;
#endif

   return os.str();
}

void
hdu::correct_errors(bit_vector& frame)
{
   apply_golay_correction(frame);
   apply_rs_correction(frame);
}

void
hdu::apply_golay_correction(bit_vector& frame)
{
#if 0
   static itpp::Extended_Golay golay;
   static const size_t NOF_GOLAY_CODEWORDS = 36, GOLAY_CODEWORD_SZ = 18;
   static const size_t GOLAY_CODEWORDS[nof_golay_codewords][golay_codeword_sz] = {
      { 119, 118, 117, 116, 115, 114, 131, 130, 129, 128, 127, 126, 125, 124, 123, 122, 121, 120 },
      { 137, 136, 135, 134, 133, 132, 151, 150, 149, 148, 147, 146, 145, 144, 141, 140, 139, 138 },
      { 157, 156, 155, 154, 153, 152, 169, 168, 167, 166, 165, 164, 163, 162, 161, 160, 159, 158 },
      { 175, 174, 173, 172, 171, 170, 187, 186, 185, 184, 183, 182, 181, 180, 179, 178, 177, 176 },
      { 193, 192, 191, 190, 189, 188, 205, 204, 203, 202, 201, 200, 199, 198, 197, 196, 195, 194 },
      { 211, 210, 209, 208, 207, 206, 225, 224, 223, 222, 221, 220, 219, 218, 217, 216, 213, 212 },
      { 231, 230, 229, 228, 227, 226, 243, 242, 241, 240, 239, 238, 237, 236, 235, 234, 233, 232 },
      { 249, 248, 247, 246, 245, 244, 261, 260, 259, 258, 257, 256, 255, 254, 253, 252, 251, 250 },
      { 267, 266, 265, 264, 263, 262, 279, 278, 277, 276, 275, 274, 273, 272, 271, 270, 269, 268 },
      { 285, 284, 283, 282, 281, 280, 299, 298, 297, 296, 295, 294, 293, 292, 291, 290, 289, 288 },
      { 305, 304, 303, 302, 301, 300, 317, 316, 315, 314, 313, 312, 311, 310, 309, 308, 307, 306 },
      { 323, 322, 321, 320, 319, 318, 335, 334, 333, 332, 331, 330, 329, 328, 327, 326, 325, 324 },
      { 341, 340, 339, 338, 337, 336, 353, 352, 351, 350, 349, 348, 347, 346, 345, 344, 343, 342 },
      { 361, 360, 357, 356, 355, 354, 373, 372, 371, 370, 369, 368, 367, 366, 365, 364, 363, 362 },
      { 379, 378, 377, 376, 375, 374, 391, 390, 389, 388, 387, 386, 385, 384, 383, 382, 381, 380 },
      { 397, 396, 395, 394, 393, 392, 409, 408, 407, 406, 405, 404, 403, 402, 401, 400, 399, 398 },
      { 415, 414, 413, 412, 411, 410, 427, 426, 425, 424, 423, 422, 421, 420, 419, 418, 417, 416 },
      { 435, 434, 433, 432, 429, 428, 447, 446, 445, 444, 443, 442, 441, 440, 439, 438, 437, 436 },
      { 453, 452, 451, 450, 449, 448, 465, 464, 463, 462, 461, 460, 459, 458, 457, 456, 455, 454 },
      { 471, 470, 469, 468, 467, 466, 483, 482, 481, 480, 479, 478, 477, 476, 475, 474, 473, 472 },
      { 489, 488, 487, 486, 485, 484, 501, 500, 499, 498, 497, 496, 495, 494, 493, 492, 491, 490 },
      { 509, 508, 507, 506, 505, 504, 521, 520, 519, 518, 517, 516, 515, 514, 513, 512, 511, 510 },
      { 527, 526, 525, 524, 523, 522, 539, 538, 537, 536, 535, 534, 533, 532, 531, 530, 529, 528 },
      { 545, 544, 543, 542, 541, 540, 557, 556, 555, 554, 553, 552, 551, 550, 549, 548, 547, 546 },
      { 563, 562, 561, 560, 559, 558, 577, 576, 573, 572, 571, 570, 569, 568, 567, 566, 565, 564 },
      { 583, 582, 581, 580, 579, 578, 595, 594, 593, 592, 591, 590, 589, 588, 587, 586, 585, 584 },
      { 601, 600, 599, 598, 597, 596, 613, 612, 611, 610, 609, 608, 607, 606, 605, 604, 603, 602 },
      { 619, 618, 617, 616, 615, 614, 631, 630, 629, 628, 627, 626, 625, 624, 623, 622, 621, 620 },
      { 637, 636, 635, 634, 633, 632, 651, 650, 649, 648, 645, 644, 643, 642, 641, 640, 639, 638 },
      { 657, 656, 655, 654, 653, 652, 669, 668, 667, 666, 665, 664, 663, 662, 661, 660, 659, 658 },
      { 675, 674, 673, 672, 671, 670, 687, 686, 685, 684, 683, 682, 681, 680, 679, 678, 677, 676 },
      { 693, 692, 691, 690, 689, 688, 705, 704, 703, 702, 701, 700, 699, 698, 697, 696, 695, 694 },
      { 711, 710, 709, 708, 707, 706, 725, 724, 723, 722, 721, 720, 717, 716, 715, 714, 713, 712 },
      { 731, 730, 729, 728, 727, 726, 743, 742, 741, 740, 739, 738, 737, 736, 735, 734, 733, 732 },
      { 749, 748, 747, 746, 745, 744, 761, 760, 759, 758, 757, 756, 755, 754, 753, 752, 751, 750 },
      { 767, 766, 765, 764, 763, 762, 779, 778, 777, 776, 775, 774, 773, 772, 771, 770, 769, 768 }
   };

   for(size_t i = 0; i < NOF_GOLAY_CODEWORDS; ++i) {
      // ToDo: 
   }
#endif
}

void
hdu::apply_rs_correction(bit_vector& frame)
{
#if 0
   static itpp::Reed_Solomon rs(6, 8, true);

   const size_t rs_codeword[][6] = {
   };
   const size_t nof_codeword_bits = sizeof(codeword_bits) / sizeof(codeword_bits[0]);

#endif
}

uint16_t
hdu::frame_size_max() const
{
   return 792;
}

string
hdu::algid_str() const
{
   const size_t ALGID_BITS[] = {
      356, 357, 360, 361, 374, 375, 376, 377
   };
   const size_t ALGID_BITS_SZ = sizeof(ALGID_BITS) / sizeof(ALGID_BITS[0]);
   uint8_t algid = extract(frame_body(), ALGID_BITS, ALGID_BITS_SZ);
   return lookup(algid, ALGIDS, ALGIDS_SZ);
}

std::string
hdu::mi_str() const
{
   const size_t MI_BITS[] = {
      114, 115, 116, 117, 118, 119, 132, 133,
      134, 135, 136, 137, 152, 153, 154, 155,
      156, 157, 170, 171, 172, 173, 174, 175,
      188, 189, 190, 191, 192, 193, 206, 207,
      208, 209, 210, 211, 226, 227, 228, 229,
      230, 231, 244, 245, 246, 247, 248, 249,
      262, 263, 264, 265, 266, 267, 280, 281,
      282, 283, 284, 285, 300, 301, 302, 303,
      304, 305, 318, 319, 320, 321, 322, 323,
   };
   const size_t MI_BITS_SZ = sizeof(MI_BITS) / sizeof(MI_BITS[0]);

   uint8_t mi[9];
   extract(frame_body(), MI_BITS, MI_BITS_SZ, mi);
   ostringstream os;
   for(size_t i = 0; i < (sizeof(mi) / sizeof(mi[0])); ++i) {
      uint16_t octet = mi[i];
      os << hex << setfill('0') << setw(2) << octet;
   }
   return os.str();
}

string
hdu::mfid_str() const
{
   const size_t MFID_BITS[] = {
      336, 337, 338, 339, 340, 341, 354, 355,
   };
   const size_t MFID_BITS_SZ = sizeof(MFID_BITS) / sizeof(MFID_BITS_SZ);
   uint8_t mfid = extract(frame_body(), MFID_BITS, MFID_BITS_SZ);
   return lookup(mfid, MFIDS, MFIDS_SZ);
}

string
hdu::nac_str() const
{
   const size_t NAC_BITS[] = {
      48,49,50,51,52,53,54,55,56,57,58,59
   };
   const size_t NAC_BITS_SZ = sizeof(NAC_BITS) / sizeof(NAC_BITS[0]);
   uint32_t nac = extract(frame_body(), NAC_BITS, NAC_BITS_SZ);
   return lookup(nac, NACS, NACS_SZ);
}
