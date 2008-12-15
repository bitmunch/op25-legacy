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
#include <itpp/comm/egolay.h>
#include <itpp/comm/reedsolomon.h>

hdu::hdu(const_bit_vector& frame_body) :
   abstract_data_unit(frame_body)
{
}

hdu::~hdu()
{
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
   // static itpp::Extended_Golay golay;

}

void
hdu::apply_rs_correction(bit_vector& frame)
{
   // static itpp::Reed_Solomon rs(63, 47, 17, true);
}

uint16_t
hdu::frame_size_encoded() const
{
   return 792;
}

#if 0
const uint16_t*
hdu::interleaving() const
{
   static const uint16_t int_sched[] = {
/*
0	FS(47)
1	FS(46)
2	FS(45)
3	FS(44)
4	FS(43)
5	FS(42)
6	FS(41)
7	FS(40)
8	FS(39)
9	FS(38)
10	FS(37)
11	FS(36)
12	FS(35)
13	FS(34)
14	FS(33)
15	FS(32)
16	FS(31)
17	FS(30)
18	FS(29)
19	FS(28)
20	FS(27)
21	FS(26)
22	FS(25)
23	FS(24)
24	FS(23)
25	FS(22)
26	FS(21)
27	FS(20)
28	FS(19)
29	FS(18)
30	FS(17)
31	FS(16)
32	FS(15)
33	FS(14)
34	FS(13)
35	FS(12)
36	FS(11)
37	FS(10)
38	FS(9)
39	FS(8)
40	FS(7)
41	FS(6)
42	FS(5)
43	FS(4)
44	FS(3)
45	FS(2)
46	FS(1)
47	FS(0)
48	NAC(11)
49	NAC(10)
50	NAC(9)
51	NAC(8)
52	NAC(7)
53	NAC(6)
54	NAC(5)
55	NAC(4)
56	NAC(3)
57	NAC(2)
58	NAC(1)
59	NAC(0)
60	DUID(3)
61	DUID(2)
62	DUID(1)
63	DUID(0)
114	MI(71)
115	MI(70)
116	MI(69)
117	MI(68)
118	MI(67)
119	MI(66)
132	MI(65)
133	MI(64)
134	MI(63)
135	MI(62)
136	MI(61)
137	MI(60)
152	MI(59)
153	MI(58)
154	MI(57)
155	MI(56)
156	MI(55)
157	MI(54)
170	MI(53)
171	MI(52)
172	MI(51)
173	MI(50)
174	MI(49)
175	MI(48)
188	MI(47)
189	MI(46)
190	MI(45)
191	MI(44)
192	MI(43)
193	MI(42)
206	MI(41)
207	MI(40)
208	MI(39)
209	MI(38)
210	MI(37)
211	MI(36)
226	MI(35)
227	MI(34)
228	MI(33)
229	MI(32)
230	MI(31)
231	MI(30)
244	MI(29)
245	MI(28)
246	MI(27)
247	MI(26)
248	MI(25)
249	MI(24)
262	MI(23)
263	MI(22)
264	MI(21)
265	MI(20)
266	MI(19)
267	MI(18)
280	MI(17)
281	MI(16)
282	MI(15)
283	MI(14)
284	MI(13)
285	MI(12)
300	MI(11)
301	MI(10)
302	MI(9)
303	MI(8)
304	MI(7)
305	MI(6)
318	MI(5)
319	MI(4)
320	MI(3)
321	MI(2)
322	MI(1)
323	MI(0)
336	MFID(7)
337	MFID(6)
338	MFID(5)
339	MFID(4)
340	MFID(3)
341	MFID(2)
354	MFID(1)
355	MFID(0)
356	ALGID(7)
357	ALGID(6)
360	ALGID(5)
361	ALGID(4)
374	ALGID(3)
375	ALGID(2)
376	ALGID(1)
377	ALGID(0)
378	KID(15)
379	KID(14)
392	KID(13)
393	KID(12)
394	KID(11)
395	KID(10)
396	KID(9)
397	KID(8)
410	KID(7)
411	KID(6)
412	KID(5)
413	KID(4)
414	KID(3)
415	KID(2)
428	KID(1)
429	KID(0)
432	TGID(15)
433	TGID(14)
434	TGID(13)
435	TGID(12)
448	TGID(11)
449	TGID(10)
450	TGID(9)
451	TGID(8)
452	TGID(7)
453	TGID(6)
466	TGID(5)
467	TGID(4)
468	TGID(3)
469	TGID(2)
470	TGID(1)
471	TGID(0)

70	SS(1)
71	SS(0)
142	SS(1)
143	SS(0)
214	SS(1)
215	SS(0)
286	SS(1)
287	SS(0)
358	SS(1)
359	SS(0)
430	SS(1)
431	SS(0)
502	SS(1)
503	SS(0)
574	SS(1)
575	SS(0)
646	SS(1)
647	SS(0)
718	SS(1)
719	SS(0)
790	SS(1)
791	SS(0)
*/
   };
   return sched;
}
#endif
