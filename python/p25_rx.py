#!/usr/bin/env python

# Copyright 2008 Steve Glass
# 
# This file is part of OP25.
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

import sys
import threading
import wx
import wx.wizard

from gnuradio import audio, eng_notation, fsk4, gr, gru, op25
from gnuradio.eng_option import eng_option
from gnuradio.wxgui import stdgui2, fftsink2, scopesink2
from math import pi
from optparse import OptionParser
from usrpm import usrp_dbid

class p25_rx_block (stdgui2.std_top_block):

    def __init__(self, frame, panel, vbox, argv):
        stdgui2.std_top_block.__init__(self, frame, panel, vbox, argv)

        # Create the GUI components
        # 
        self.frame = frame
        self.frame.CreateStatusBar()
        self.frame.SetStatusText("")
        self.panel = panel
        self.notebook = wx.Notebook(self.panel)
        vbox.Add(self.notebook, 1, wx.EXPAND)       

        menubar = frame.GetMenuBar()
        file_menu = menubar.GetMenu(0)
        self.file_open = file_menu.Insert(0, 201, u'&Open...\tCtrl-O', 'Open file', wx.ITEM_NORMAL)
        frame.Bind(wx.EVT_MENU, self.on_file_open, self.file_open)
        self.file_capture = file_menu.Insert(1, 202, u'&Capture...\tCtrl-C', 'Capture from radio', wx.ITEM_NORMAL)
        frame.Bind(wx.EVT_MENU, self.on_file_capture, self.file_capture)
        file_menu.InsertSeparator(2)
        self.file_save = file_menu.Insert(3, 203, u'Save\tCtrl-S', 'Save file.', wx.ITEM_NORMAL)
        self.file_save.Enable(False)
        frame.Bind(wx.EVT_MENU, self.on_file_save, self.file_save)
        self.file_save_as = file_menu.Insert(4, 204, u'Save &As\tShift-Ctrl-S', 'Save file', wx.ITEM_NORMAL)
        self.file_save_as.Enable(False)
        frame.Bind(wx.EVT_MENU, self.on_file_save_as, self.file_save_as)
        file_menu.InsertSeparator(5)
        self.file_properties = file_menu.Insert(6, 203, u'Properties...\tAlt-Enter', 'File properties.', wx.ITEM_NORMAL)
        self.file_properties.Enable(False)
        frame.Bind(wx.EVT_MENU, self.on_file_properties, self.file_properties)
        file_menu.InsertSeparator(7)
        self.file_close = file_menu.Insert(8, 202, u'Close\tCtrl-W', 'Close file', wx.ITEM_NORMAL)
        self.file_close.Enable(False)
        frame.Bind(wx.EVT_MENU, self.on_file_close, self.file_close)

#         toolbar = wx.ToolBar(frame, -1, style = wx.TB_DOCKABLE | wx.TB_HORIZONTAL)
#         open_img = wx.Bitmap(u'images/open.png', wx.BITMAP_TYPE_PNG)
#         toolbar.AddTool(201, u"Open", open_img)
#         frame.SetToolBar(toolbar)

        # Any command line args (avoid entering STOPPED state)
        #
        parser = OptionParser(option_class=eng_option)
        # either these
        parser.add_option("-i", "--input", default=None, help="set input file")
        parser.add_option("-d", "--decim", type="int", default=256, help="set decimation")
        parser.add_option("-l", "--loop", action="store_true", default=False, help="loop input file")
        # or these
        parser.add_option("-f", "--frequency", type="eng_float", default=0.0, help="set center frequency", metavar="Hz")
        parser.add_option("-c", "--channel-decim", type="int", default=1, help="channel decimation")
        # or these
        parser.add_option("-s", "--sound-card", type="string", default=None, help="device name (e.g. hw:0,0 or /dev/dsp)")
        parser.add_option("-r", "--sample-rate", type="int", default=48000, help="sound card input sample rate")
        # go parse
        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        # Build flow graph
        #
        usrp_rate = 64000000
        source_rate = usrp_rate // options.decim

        if options.input:
            file = gr.file_source(gr.sizeof_gr_complex, options.input, options.loop)
            throttle = gr.throttle(gr.sizeof_gr_complex, source_rate);
            self.connect(file, throttle)
            self.source = throttle
        elif options.sound_card:
            soundcard = audio.source(options.sample_rate, options.sound_card)
            # 
            complex_conv = gr.float_to_complex()
            self.connect(soundcard, complex_conv)
            self.source = complex_conv
        elif options.frequency:
            usrp = usrp.source_c(0)
            subdev = usrp.pick_subdev(usrp, (usrp_dbid.TV_RX, usrp_dbid.TV_RX_REV_2, udrp.dbid.TV_RX_REV_3))
            usrp.set_mux(usrp.determine_rx_mux_value(usrp, subdev))
            usrp.set_decim_rate(options.decim)
#             if gain is None:
#                 g = self._subdev.gain_range()
#                 gain = (g[0]+g[1])/2.0
#             self._subdev.set_gain(gain)
            usrp.tune(options.frequency)
            junk = usrp.tune(usrp, 0, subdev, frequency)
            self.source = usrp
        else:
            print "no input specified!"
            exit(1)


        # Create the flow graph
        #
        self.spectrum = fftsink2.fft_sink_c(self.notebook, fft_size=512, sample_rate=source_rate, average=True, peak_hold=True)
        self.spectrum_plotter = self.spectrum.win.plot
        self.notebook.AddPage(self.spectrum.win, "RF Spectrum")
        self.spectrum_plotter.Bind(wx.EVT_LEFT_DOWN, self.on_spectrum_left_click)
        self.connect(self.source, self.spectrum)

        self.channel_offset = 0.0
        self.channel_decim = source_rate // 125000
        self.channel_rate = source_rate // self.channel_decim
        trans_width = 12.5e3 / 2;
        trans_centre = trans_width + (trans_width / 2)
        coeffs = gr.firdes.low_pass(1.0, source_rate, trans_centre, trans_width, gr.firdes.WIN_HANN)
        self.channel_filter = gr.freq_xlating_fir_filter_ccf(self.channel_decim, coeffs, 0.0, source_rate)
        self.connect(self.source, self.channel_filter);
        self.set_channel_offset(0.0, 0, self.spectrum.win._units)

        if True:
            self.channel_scope = scopesink2.scope_sink_c(self.notebook, sample_rate=self.channel_rate, v_scale=5, t_scale=0.001)
            self.notebook.AddPage(self.channel_scope.win, "Channel")
            self.connect(self.channel_filter, self.channel_scope)

        squelch_db = 0
        self.squelch = gr.pwr_squelch_cc(squelch_db, 1e-3, 0, True)
        self.set_squelch_threshold(squelch_db)
        self.connect(self.channel_filter, self.squelch)

        self.symbol_rate = 4800
        self.symbol_deviation = 600.0
        fm_demod_gain = self.channel_rate / (2.0 * pi * self.symbol_deviation)
        self.fm_demod = gr.quadrature_demod_cf(fm_demod_gain)
        self.connect(self.squelch, self.fm_demod)

        symbol_decim = 1
        symbol_coeffs = gr.firdes.root_raised_cosine(1.0, self.channel_rate, self.symbol_rate, 0.2, 500)
        self.symbol_filter = gr.fir_filter_fff(symbol_decim, symbol_coeffs)
        self.connect(self.fm_demod, self.symbol_filter)

        if True:
            self.signal_scope = scopesink2.scope_sink_f(self.notebook, sample_rate=self.channel_rate, v_scale=5, t_scale=0.001)
            self.notebook.AddPage(self.signal_scope.win, "C4FM Signal")
            self.connect(self.symbol_filter, self.signal_scope)

        autotuneq = gr.msg_queue(2)
        self.demod_watcher = demod_watcher(autotuneq, self.adjust_channel_offset)
        self.demod_fsk4 = fsk4.demod_ff(autotuneq, self.channel_rate, self.symbol_rate)
        self.connect(self.symbol_filter, self.demod_fsk4)

        if True:
            self.symbol_scope = scopesink2.scope_sink_f(self.notebook, sample_rate=self.symbol_rate, frame_decim=1, v_scale=1, t_scale=0.05)
            self.notebook.AddPage(self.symbol_scope.win, "Demodulated Symbols")
            self.connect(self.demod_fsk4, self.symbol_scope)
            self.symbol_scope.win.set_format_plus()

        self.p25_decoder = op25.decoder_ff()
        self.connect(self.demod_fsk4, self.p25_decoder)
        self.frame.SetStatusText("TUN/TAP: " + self.p25_decoder.device_name())

#       self.sink = gr.null_sink(gr.sizeof_float)
#       self.connect(self.p25_decoder, self.sink)

    def adjust_channel_offset(self, delta_hz):
        max_delta_hz = 6000.0
        delta_hz *= self.symbol_deviation      
        delta_hz = max(delta_hz, -max_delta_hz)
        delta_hz = min(delta_hz, max_delta_hz)
        self.channel_filter.set_center_freq(self.channel_offset - delta_hz)

    def on_spectrum_left_click(self, event):
        # set frequency
        x,y = self.spectrum_plotter.GetXY(event)
        xmin, xmax = self.spectrum_plotter.GetXCurrentRange()
        x = min(x, xmax)
        x = max(x, xmin)
        scale_factor = self.spectrum.win._scale_factor
        chan_width = 12.5e3
        x /= scale_factor
        x += chan_width / 2
        x  = (x // chan_width) * chan_width
        self.set_channel_offset(x, scale_factor, self.spectrum.win._units)
        # set squelch threshold
        ymin, ymax = self.spectrum_plotter.GetYCurrentRange()
        y = min(y, ymax)
        y = max(y, ymin)
        self.set_squelch_threshold(int(y))

    def on_file_open(self, event):
        dialog = wx.FileDialog(self.frame, "Choose a capture file:", wildcard="*.dat", style=wx.OPEN)
        if dialog.ShowModal() == wx.ID_OK:
            file = dialog.GetPath()
            dialog.Destroy()
            # ToDo: extract the decimation factor
            # ToDo: stop and change the flow graph

    def on_file_capture(self, event):
        todo = True
        # ToDo: start the capture druid
        # ToDo: stop and change the flow graph

    def on_file_save(self, event):
        todo = True
        # ToDo: stop the flow graph
        # ToDo: decide if we need to show file save dialog
        # ToDo: rename the file

    def on_file_save_as(self, event):
        todo = True
        # ToDo: stop the flow graph
        # ToDo: decide if we need to show file save dialog
        # ToDo: rename the file

    def on_file_properties(self, event):
        todo = True
        # ToDo: show what data we have about the file (name, source, decimation factor, date(?), size(?),)

    def on_file_close(self, event):
        todo = True
        # ToDo: stop the flow graph
        # ToDo: decide if we need to save
        # ToDo: ask "are you sure?"

    def set_channel_offset(self, offset_hz, scale, units):
        self.channel_offset = -offset_hz
        self.channel_filter.set_center_freq(self.channel_offset)
        self.frame.SetStatusText("Channel offset: " + str(offset_hz * scale) + units, 1)

    def set_squelch_threshold(self, squelch_db):
        self.squelch.set_threshold(squelch_db)
        self.frame.SetStatusText("Squelch: " + str(squelch_db) + "dB", 2)

    def set_state(self, new_state):
        self.state = new_state
        if self.state == STOPPED:
            self.file_save.Enable(False)
            self.file_save_as.Enable(False)
            self.file_properties.Enable(False)
            self.file_close.Enable(False)
        elif self.state == RUNNING:
            self.file_save.Enable(False)
            self.file_save_as.Enable(False)
            self.file_properties.Enable(True)
            self.file_close.Enable(True)
        elif self.state == CAPTURING:
            self.file_save.Enable(True)
            self.file_save_as.Enable(True)
            self.file_properties.Enable(True)
            self.file_close.Enable(True)

    def dump(self, object):
        print object.__class__.__name__
        methodList = [method for method in dir(object) if callable(getattr(object, method))]
        processFunc = (lambda s: " ".join(s.split())) or (lambda s: s)
        print "\n".join(["%s %s" %
                         (method.ljust(10),
                          processFunc(str(getattr(object, method).__doc__)))
                         for method in methodList])

class demod_watcher(threading.Thread):

    def __init__(self, msgq,  callback, **kwds):
        threading.Thread.__init__ (self, **kwds)
        self.setDaemon(1)
        self.msgq = msgq
        self.callback = callback
        self.keep_running = True
        self.start()

    def run(self):
        while(self.keep_running):
            msg = self.msgq.delete_head()
            frequency_correction = msg.arg1()
            self.callback(frequency_correction)

if '__main__' == __name__:
    app = stdgui2.stdapp(p25_rx_block, "APCO P25 Receiver", 3)
    app.MainLoop()
