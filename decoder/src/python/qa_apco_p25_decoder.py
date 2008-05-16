#!/usr/bin/env python
#
# Copyright 2008 Stevie Glass
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

class qa_apco_p25_decoder (gr_unittest.TestCase):

    def setUp(self):
        self.fg = gr.flow_graph ()

    def tearDown(self):
        self.fg = None

    def test_constuct_apco_p25_decoder_f(self):
        framing_sequence = ()
        src = gr.vector_source_f(framing_sequence)
        p25 = gr.apco_25_decoder_f()
        self.fg.connect(src, p25)
        self.fg.run()
        # no meaningful external test at present
        
if __name__ == '__main__':
    gr_unittest.main ()
