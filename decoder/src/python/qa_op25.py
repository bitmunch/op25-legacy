#!/usr/bin/env python
#
# Copyright 2008 Steve Glass
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
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with OP25; see the file COPYING.  If not, write to the Free
# Software Foundation, Inc., 51 Franklin Street, Boston, MA
# 02110-1301, USA.
# 

from gnuradio import gr, gr_unittest
#from gnuradio import op25
import op25

# import os
#
# print 'Blocked waiting for GDB attach (pid = %d)' % (os.getpid(),)
# raw_input("Press Enter to continue...")

class qa_op25(gr_unittest.TestCase):

    def setUp(self):
        self.fg = gr.flow_graph ()

    def tearDown(self):
        self.fg = None

#     def test_op25_decoder_f_ctor(self):
#         msgq = gr.msg_queue()
#         self.fg.connect(src, p25)
#         p25 = op25.decoder_f(msgq)
#         # assert not null

    def test_correlator(self):
        framing_sequence = (3, 3, 3, 3, 3, -3, 3, 3, -3, -3, 3, 3, -3, -3, -3, -3, 3, -3, 3, -3, -3, -3, -3, -3)
        src = gr.vector_source_f(framing_sequence, False)
        msgq = gr.msg_queue()
        p25 = op25.decoder_f(msgq)
        self.fg.connect(src, p25)
        self.fg.run()
        # check the msgq output (needs separate thread and terminate the flow graph)
        
if __name__ == '__main__':
    gr_unittest.main ()
