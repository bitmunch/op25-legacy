#!/usr/bin/env python

# Copyright 2011 KA1RBI
# 
# This file is part of OP25 and part of GNU Radio
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

import os
import pickle
import sys
import threading
import wx
import wx.html
import wx.wizard
import math
import numpy
import time
import re

from gnuradio import audio, eng_notation, fsk4, gr, gru, repeater
from gnuradio import blks2
from gnuradio.eng_option import eng_option
from gnuradio.wxgui import stdgui2, fftsink2, scopesink2, form
from math import pi
from optparse import OptionParser
from usrpm import usrp_dbid

import gnuradio.wxgui.plot as plot

speeds = [300, 600, 900, 1200, 1440, 1800, 1920, 2400, 2880, 3200, 3600, 3840, 4000, 4800, 6400, 7200, 8000, 9600, 14400, 19200]

# The P25 receiver
#
class p25_rx_block (stdgui2.std_top_block):

    # Initialize the P25 receiver
    #
    def __init__(self, frame, panel, vbox, argv):

        stdgui2.std_top_block.__init__(self, frame, panel, vbox, argv)

        # command line argument parsing
        parser = OptionParser(option_class=eng_option)
        parser.add_option("-a", "--audio", action="store_true", default=False, help="use direct audio input")
        parser.add_option("-I", "--audio-input", type="string", default="", help="pcm input device name.  E.g., hw:0,0 or /dev/dsp")
        parser.add_option("-i", "--input", default=None, help="input file name")
        parser.add_option("-b", "--excess-bw", type="eng_float", default=0.2, help="offset frequency", metavar="Hz")
        parser.add_option("-c", "--calibration", type="eng_float", default=0.0, help="offset frequency", metavar="Hz")
        parser.add_option("-o", "--offset", type="eng_float", default=0.0, help="offset frequency", metavar="Hz")
        parser.add_option("-C", "--costas-alpha", type="eng_float", default=0.10, help="offset frequency", metavar="Hz")
        parser.add_option("-f", "--frequency", type="eng_float", default=0.0, help="USRP center frequency", metavar="Hz")
        parser.add_option("-d", "--decim", type="int", default=200, help="source decimation factor")
        parser.add_option("-v", "--verbosity", type="int", default=10, help="message debug level")
        parser.add_option("-w", "--wait", action="store_true", default=False, help="block on startup")
        parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=(0, 0), help="select USRP Rx side A or B (default=A)")
        parser.add_option("-g", "--gain", type="eng_float", default=None, help="set USRP gain in dB (default is midpoint) (or audio gain)")
        parser.add_option("-G", "--gain-mu", type="eng_float", default=0.025, help="gardner gain")
        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        self.channel_rate = 0
        self.baseband_input = False

        # do we have a USRP?
        try:
            self.usrp = None
            from gnuradio import usrp
            self.usrp = usrp.source_c()
            self.channel_rate = self.usrp.adc_freq() / options.decim
        except Exception:
            ignore = True

        if options.audio:
            self.channel_rate = 48000
            self.baseband_input = True

        # setup (read-only) attributes
        self.symbol_rate = 4800
        self.symbol_deviation = 600.0
        self.basic_rate = 48000
        _default_speed = 4800

        # keep track of flow graph connections
        self.cnxns = []

        self.datascope_raw_input = False
        self.data_scope_connected = False

        self.options = options

        for i in xrange(len(speeds)):
            if speeds[i] == _default_speed:
                self.current_speed = i

        # initialize the UI
        # 
        self.__init_gui(frame, panel, vbox)

        # wait for gdb
        if options.wait:
            print 'Ready for GDB to attach (pid = %d)' % (os.getpid(),)
            raw_input("Press 'Enter' to continue...")

        # configure specified data source
        if options.input:
            self.open_file(options.input)
        elif options.frequency:
            self._set_state("CAPTURING")
            self.open_usrp(options.rx_subdev_spec, options.decim, options.gain, options.frequency + options.offset, True)
        elif options.audio:
            self._set_state("CAPTURING")
            self.open_audio(self.channel_rate, options.gain, options.audio_input)
            # skip past unused FFT spectrum plot
            self.notebook.AdvanceSelection()
        else:
            self._set_state("STOPPED")

    # setup common flow graph elements
    #
    def __build_graph(self, source, capture_rate):
        global speeds
        # tell the scope the source rate
        self.spectrum.set_sample_rate(capture_rate)
        # channel filter
        self.channel_offset = 0.0
        channel_decim = capture_rate // self.channel_rate
        channel_rate = capture_rate // channel_decim
        trans_width = 12.5e3 / 2;
        trans_centre = trans_width + (trans_width / 2)
        sps = int(self.basic_rate // self.symbol_rate)

        symbol_decim = 1
        ntaps = 11 * 10
#       rrc_coeffs = gr.firdes.root_raised_cosine(1.0, self.basic_rate, self.basic_rate * 0.1, 0.2, ntaps)
        rrc_coeffs = (1.0/sps,)*sps
        self.symbol_filter = gr.fir_filter_fff(symbol_decim, rrc_coeffs)

        autotuneq = gr.msg_queue(2)
        self.fsk4_demod = fsk4.demod_ff(autotuneq, self.basic_rate, self.symbol_rate)

        self.null_sym = gr.null_sink(gr.sizeof_float)

        levels = [ -2.0, 0.0, 2.0, 4.0 ]
        self.slicer = repeater.fsk4_slicer_fb(levels)

        self.buffer = gr.copy(gr.sizeof_float)

        msgq = gr.msg_queue(2)
        self.sink_s = repeater.p25_frame_assembler('', 0, self.options.verbosity, 0, 0, 0, msgq)

        if self.baseband_input:
            gain = 50.0
        else:
            gain = 1.0
        self.baseband_amp = gr.multiply_const_ff(gain)
        self.real_amp = gr.multiply_const_ff(0.2)

        self.sinkf = gr.file_sink(gr.sizeof_float, "sinkf.dat")

#       self.connect_data_scope(True)

        self.fft_state  = False
        self.c4fm_state = False
        self.cscope_state = False
        self.fscope_state = False
        self.corr_state = False
        self.fsk4_demod_connected = False
        self.psk_demod_connected = False
        self.fsk4_demod_mode = True
        self.corr_i_chan = False

        if self.baseband_input:
            self.rescale = gr.multiply_const_ff( 1.0 ) # dummy for compat
            self.float_resamplers = []
            for i in xrange(len(speeds)):
                speed = speeds[i] * sps
		lcm = gru.lcm(speed, capture_rate)
                interp = int(lcm // capture_rate)
                decim = int(lcm // speed)
                resamp = blks2.rational_resampler_fff(interp,decim)
                self.float_resamplers.append(resamp)
            self.set_connection(c4fm=1)
        else:	# complex input
            coeffs = gr.firdes.low_pass(1.0, capture_rate, trans_centre, trans_width, gr.firdes.WIN_HANN)
            self.channel_filter = gr.freq_xlating_fir_filter_ccf(channel_decim, coeffs, 0.0, capture_rate)
            self.set_channel_offset(0.0, 0, "")
            # local osc
            self.lo = gr.sig_source_c (channel_rate, gr.GR_SIN_WAVE, 0.0, 1.0, 0)
            self.mixer = gr.multiply_cc()
            self.decim_sink = gr.file_sink(gr.sizeof_gr_complex, "32k-complex.dat")
            lpf_coeffs = gr.firdes.low_pass(1.0, self.channel_rate, 12000, 1200, gr.firdes.WIN_HANN)
            self.lpf = gr.fir_filter_ccf(1, lpf_coeffs)

            self.to_real = gr.complex_to_real()

            nphases = 64
            frac_bw = 0.25
            samp_rate = 320e3
            rs_taps = gr.firdes.low_pass(nphases, nphases, frac_bw, 0.5-frac_bw)

            self.arb_resampler = blks2.pfb_arb_resampler_ccf(
               float(self.basic_rate)/samp_rate,
               (rs_taps),
               nphases,
            )

            fm_demod_gain = self.basic_rate / (2.0 * pi * self.symbol_deviation)
            self.fm_demod = gr.quadrature_demod_cf(fm_demod_gain)

            rrc_taps = gr.firdes.root_raised_cosine(
                1.0, # gain  (sps since we're interpolating by 
                10, # sampling rate
                1.0,                      # symbol rate
                self.options.excess_bw,   # excess bandwidth (roll-off factor)
                ntaps)

            self.symbol_filter_c = gr.interp_fir_filter_ccf(1, rrc_taps)
            samp_rate = self.symbol_rate
            self.resamplers = []

            for speed in speeds:
                resampler = blks2.pfb_arb_resampler_ccf(
                   float(speed)/float(samp_rate),
                   (rs_taps),
                   nphases,
                )
                self.resamplers.append(resampler)

            theta = pi

            gain_mu= self.options.gain_mu
            omega = 10
            gain_omega = 0.1  * gain_mu * gain_mu

            alpha = self.options.costas_alpha
            beta = 0.125 * alpha * alpha
            fmin = -0.025
            fmax =  0.025

            self.clock = repeater.gardner_costas_cc(10.0, gain_mu, gain_omega, alpha,  beta, fmax, fmin)

            self.agc = gr.feedforward_agc_cc(16, 1.0)

            # Perform Differential decoding on the constellation
            self.diffdec = gr.diff_phasor_cc()

            # take angle of the difference (in radians)
            self.to_float = gr.complex_to_arg()

            # convert from radians such that signal is in -3/-1/+1/+3
            self.rescale = gr.multiply_const_ff( (1 / (pi / 4)) )

            # connect it all up
            self.__connect([[source, (self.mixer, 0)],
                            [self.lo, (self.mixer, 1)]])
            self.set_connection(fft=1)

    # Connect up the flow graph
    #
    def __connect(self, cnxns):
        for l in cnxns:
            for b in l:
                if b == l[0]:
                    p = l[0]
                else:
                    self.connect(p, b)
                    p = b
        self.cnxns.extend(cnxns)

    # Disconnect the flow graph
    #
    def __disconnect(self):
        for l in self.cnxns:
            for b in l:
                if b == l[0]:
                    p = l[0]
                else:
                    self.disconnect(p, b)
                    p = b
        self.cnxns = []

    def freq_slider_chg(self, evt):
        val = self.freq_control.GetValue()
        self.freq_field.SetValue(str(val))
        self.lo.set_frequency(val)

    def set_speed(self, new_speed):
     # assumes that lock is held, or that we are in init
        self.disconnect_demods()
        self.current_speed = new_speed
        self.connect_demods()

    def set_speed_old(self, new_speed):
        global speeds
     # assumes that lock is held, or that we are in init
        if new_speed == self.current_speed:
            return
        print "set new speed %d current %d" % (speeds[new_speed], self.current_speed)

        if self.baseband_input:
            self.disconnect(self.baseband_amp, self.float_resamplers[self.current_speed], self.symbol_filter)
            self.connect(self.baseband_amp, self.float_resamplers[new_speed], self.symbol_filter)
        else:
            self.disconnect(self.arb_resampler, self.resamplers[self.current_speed], self.agc)
            self.disconnect(self.resamplers[self.current_speed], self.fm_demod)
            self.connect(self.arb_resampler, self.resamplers[new_speed], self.agc)
            self.connect(self.resamplers[new_speed], self.fm_demod)

        self.current_speed = new_speed

    def set_connection(self,
                         cscope=False,
                         fscope=False,
                         fft=False,
                         corr=False,
                         c4fm=False):
     # assumes that lock is held, or that we are in init
        if corr != self.corr_state:
            self.corr_state = corr
            if corr:
                if self.corr_i_chan:
                    self.connect(self.arb_resampler, self.to_real, self.real_amp, self.correlation_scope)
                else:
                    self.connect(self.symbol_filter, self.correlation_scope)
            else:
                if self.corr_i_chan:
                    self.disconnect(self.arb_resampler, self.to_real, self.real_amp, self.correlation_scope)
                else:
                    self.disconnect(self.symbol_filter, self.correlation_scope)

        if cscope != self.cscope_state:
            self.cscope_state = cscope
            if cscope == 0:
                self.disconnect(self.diffdec, self.complex_scope)
            else:
                self.connect(self.diffdec, self.complex_scope)

        if fscope != self.fscope_state:
            self.fscope_state = fscope
            if fscope == 0:
                self.disconnect(self.buffer, self.float_scope)
            else:
                self.connect(self.buffer, self.float_scope)

        if fft != self.fft_state:
            self.fft_state = fft
            if fft == 0:
                self.disconnect(self.mixer, self.spectrum)
            else:
                self.connect(self.mixer, self.spectrum)

        if c4fm != self.c4fm_state:
            self.c4fm_state = c4fm
            if c4fm == 0:
                self.disconnect(self.symbol_filter, self.signal_scope)
            else:
                self.connect(self.symbol_filter, self.signal_scope)

    def notebook_changed(self, evt):
        sel = self.notebook.GetSelection()
        self.lock()
        self.disconnect_data_scope()
        if sel == 0:   # spectrum
            if not self.baseband_input:
                self.set_connection(fft=1)
                self.disconnect_demods()
        if sel == 1:   # c4fm
            self.set_connection(c4fm=1)
            self.connect_fsk4_demod()
        if sel == 2:   # datascope
            self.set_connection()
            self.connect_fsk4_demod()
            self.connect_data_scope()
        if sel == 3:   # constellation (complex)
            if not self.baseband_input:
                self.set_connection(cscope=1)
                self.connect_psk_demod()
        if sel == 4:   # demodulated symbols
            self.connect_demods()
            self.set_connection(fscope=1)
        if sel == 5:   # correlation
            self.set_connection(corr=1)
            self.connect_fsk4_demod()
        self.unlock()

    # initialize the UI
    # 
    def __init_gui(self, frame, panel, vbox):

        def _form_set_freq(kv):
            return self.set_freq(kv['freq'])

        self.frame = frame
        self.frame.CreateStatusBar()
        self.panel = panel
        self.vbox = vbox
        
        # setup the menu bar
        menubar = self.frame.GetMenuBar()

        # setup the "File" menu
        file_menu = menubar.GetMenu(0)
        self.file_new = file_menu.Insert(0, wx.ID_NEW)
        self.frame.Bind(wx.EVT_MENU, self._on_file_new, self.file_new)
        self.file_open = file_menu.Insert(1, wx.ID_OPEN)
        self.frame.Bind(wx.EVT_MENU, self._on_file_open, self.file_open)
        file_menu.InsertSeparator(2)
        self.file_properties = file_menu.Insert(3, wx.ID_PROPERTIES)
        self.frame.Bind(wx.EVT_MENU, self._on_file_properties, self.file_properties)
        file_menu.InsertSeparator(4)
        self.file_close = file_menu.Insert(5, wx.ID_CLOSE)
        self.frame.Bind(wx.EVT_MENU, self._on_file_close, self.file_close)

        # setup the "Edit" menu
        edit_menu = wx.Menu()
        self.edit_undo = edit_menu.Insert(0, wx.ID_UNDO)
        self.frame.Bind(wx.EVT_MENU, self._on_edit_undo, self.edit_undo)
        self.edit_redo = edit_menu.Insert(1, wx.ID_REDO)
        self.frame.Bind(wx.EVT_MENU, self._on_edit_redo, self.edit_redo)
        edit_menu.InsertSeparator(2)
        self.edit_cut = edit_menu.Insert(3, wx.ID_CUT)
        self.frame.Bind(wx.EVT_MENU, self._on_edit_cut, self.edit_cut)
        self.edit_copy = edit_menu.Insert(4, wx.ID_COPY)
        self.frame.Bind(wx.EVT_MENU, self._on_edit_copy, self.edit_copy)
        self.edit_paste = edit_menu.Insert(5, wx.ID_PASTE)
        self.frame.Bind(wx.EVT_MENU, self._on_edit_paste, self.edit_paste)
        self.edit_delete = edit_menu.Insert(6, wx.ID_DELETE)
        self.frame.Bind(wx.EVT_MENU, self._on_edit_delete, self.edit_delete)
        edit_menu.InsertSeparator(7)
        self.edit_select_all = edit_menu.Insert(8, wx.ID_SELECTALL)
        self.frame.Bind(wx.EVT_MENU, self._on_edit_select_all, self.edit_select_all)
        edit_menu.InsertSeparator(9)
        self.edit_prefs = edit_menu.Insert(10, wx.ID_PREFERENCES)
        self.frame.Bind(wx.EVT_MENU, self._on_edit_prefs, self.edit_prefs)
        menubar.Append(edit_menu, "&Edit"); # ToDo use wx.ID_EDIT stuff

        # setup the toolbar
        if True:
            self.toolbar = wx.ToolBar(frame, -1, style = wx.TB_DOCKABLE | wx.TB_HORIZONTAL)
            frame.SetToolBar(self.toolbar)
            icon_size = wx.Size(24, 24)
#           new_icon = wx.ArtProvider.GetBitmap(wx.ART_NEW, wx.ART_TOOLBAR, icon_size)
#           toolbar_new = self.toolbar.AddSimpleTool(wx.ID_NEW, new_icon, "New Capture")
#           open_icon = wx.ArtProvider.GetBitmap(wx.ART_FILE_OPEN, wx.ART_TOOLBAR, icon_size)
#           toolbar_open = self.toolbar.AddSimpleTool(wx.ID_OPEN, open_icon, "Open")
            
            self.toolbar.Realize()
        else:
            self.toolbar = None

        # setup the notebook
        self.notebook = wx.Notebook(self.panel)
        self.vbox.Add(self.notebook, 1, wx.EXPAND)       
        # add spectrum scope
        self.spectrum = fftsink2.fft_sink_c(self.notebook, sample_rate = self.channel_rate, fft_size=512, fft_rate=2, average=False, peak_hold=False)
        self.spectrum_plotter = self.spectrum.win.plotter
        self.spectrum_plotter.enable_point_label(False)
        self.spectrum_plotter.Bind(wx.EVT_LEFT_DOWN, self._on_spectrum_left_click)
        self.notebook.AddPage(self.spectrum.win, "RF Spectrum")
        # add C4FM scope
        self.signal_scope = scopesink2.scope_sink_f(self.notebook, sample_rate = self.basic_rate, v_scale=5, t_scale=0.001)
        self.signal_plotter = self.signal_scope.win.plotter
        self.notebook.AddPage(self.signal_scope.win, "C4FM Signal")
        # add datascope
        self.data_scope = datascope_sink_f(self.notebook, samples_per_symbol = 10, num_plots = 100)
        self.data_plotter = self.data_scope.win.graph
        wx.EVT_RADIOBOX(self.data_scope.win.radio_box, 11103, self.filter_select)
        wx.EVT_RADIOBOX(self.data_scope.win.radio_box_speed, 11104, self.speed_select)
        self.data_scope.win.radio_box_speed.SetSelection(self.current_speed)
        self.notebook.AddPage(self.data_scope.win, "Datascope")
        # add complex scope
        self.complex_scope = constellation_plot_c(self.notebook, title="Constellation", num_plots=250)
        self.notebook.AddPage(self.complex_scope.win, "Constellation")
        # add float scope
        self.float_scope = scopesink2.scope_sink_f(self.notebook, frame_decim=1, sample_rate=self.symbol_rate, v_scale=1, t_scale=0.05)
        self.float_plotter = self.float_scope.win.plotter
        self.float_scope.win['marker_1'] = 3.0	# set type = large dots
        self.notebook.AddPage(self.float_scope.win, "Demodulated Symbols")
        # Traffic snapshot
        # self.traffic = TrafficPane(self.notebook)
        # self.notebook.AddPage(self.traffic, "Traffic")
        # add float scope
        self.correlation_scope = correlation_plot_f(self.notebook, frame_decim=4, sps=10, v_scale=1, t_scale=0.05)
        # self.correlation_plotter = self.correlation_scope.win.plotter
        wx.EVT_RADIOBOX(self.correlation_scope.win.radio_box_corr, 11105, self.corr_select)
        self.notebook.AddPage(self.correlation_scope.win, "Correlation")
        # Setup the decoder and report the TUN/TAP device name
        msgq = gr.msg_queue(2)
        # self.decode_watcher = decode_watcher(msgq, self.traffic)
        # self.p25_decoder = op25.decoder_ff(msgq)
        # self.frame.SetStatusText("TUN/TAP: " + self.p25_decoder.device_name())

        self.notebook.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.notebook_changed)

        self.myform = myform = form.form()
        hbox = wx.BoxSizer(wx.HORIZONTAL)
        if not self.baseband_input:
            myform['freq'] = form.float_field(
                parent=self.panel, sizer=hbox, label="Frequency", weight=0,
                callback=myform.check_input_and_call(_form_set_freq, self._set_status_msg))
            myform['freq'].set_value(self.options.frequency)
        if self.baseband_input:
            min_gain = 0
            max_gain = 200
            initial_val = 50
        else:
            min_gain = 0
            max_gain = 25
            initial_val = 10
        myform['signal_gain'] = form.slider_field(parent=self.panel, sizer=hbox, label="Signal Gain",
            weight=0,
            min=min_gain, max=max_gain,
            callback=self.set_gain)
        self.myform['signal_gain'].set_value(initial_val)
        if not self.baseband_input:
            myform['freq_tune'] = form.slider_field(parent=self.panel, sizer=hbox, label="Fine Tune",
                weight=0,
                min=-3000, max=3000,
                callback=self.set_freq_tune)
            myform['demod_type'] = form.radiobox_field(parent=self.panel, sizer=hbox, label="Demod",
                weight=0, choices=['FSK4','PSK'], specify_rows=True,
                callback=self.demod_type_chg)

        vbox.Add(hbox, 0, 0)

    def set_gain(self, gain):
        if self.baseband_input:
            f = 1.0
        else:
            f = 0.1
        self.baseband_amp.set_k(float(gain) * f)

    def set_freq_tune(self, val):
        self.myform['freq_tune'].set_value(val)
        self.lo.set_frequency(val)

    def set_freq(self, target_freq):
        """
        Set the center frequency we're interested in.

        @param target_freq: frequency in Hz
        @rypte: bool

        Tuning is a two step process.  First we ask the front-end to
        tune as close to the desired frequency as it can.  Then we use
        the result of that operation and our target_frequency to
        determine the value for the digital down converter.
        """
        r = self.usrp.tune(0, self.subdev, target_freq + self.options.calibration)
        
        if r:
            self.myform['freq'].set_value(target_freq)     # update displayed va
            #if self.show_debug_info:
            #    self.myform['baseband'].set_value(r.baseband_freq)
            #    self.myform['ddc'].set_value(r.dxc_freq)
            return True

        return False

    def demod_type_chg(self, val):
        if val == 'FSK4':
            self.fsk4_demod_mode = True
        else:
            self.fsk4_demod_mode = False
        self.lock()
        self.disconnect_demods()
        notebook_sel = self.notebook.GetSelection()
        if notebook_sel == 4:	# demod symbols
             self.connect_demods()
        elif notebook_sel == 1 or notebook_sel == 2 or notebook_sel == 5:
             self.connect_fsk4_demod()
        elif notebook_sel == 3:	# constellation
             self.connect_psk_demod()

        self.unlock()

    def _set_status_msg(self, msg):
        self.frame.GetStatusBar().SetStatusText(msg, 0)

    # read capture file properties (decimation etc.)
    #
    def __read_file_properties(self, filename):
        f = open(filename, "r")
        self.info = pickle.load(f)
        ToDo = True
        f.close()

    # setup to rx from file
    #
    def __set_rx_from_file(self, filename, capture_rate):
        file = gr.file_source(gr.sizeof_gr_complex, filename, True)
        throttle = gr.throttle(gr.sizeof_gr_complex, capture_rate)
        self.__connect([[file, throttle]])
        self.__build_graph(throttle, capture_rate)

    # setup to rx from Audio
    #
    def __set_rx_from_audio(self, capture_rate):
        self.__build_graph(self.source, capture_rate)

    # setup to rx from USRP
    #
    def __set_rx_from_usrp(self, subdev_spec, decimation_rate, gain, frequency, preserve):
        from gnuradio import usrp
        # setup USRP
        self.usrp.set_decim_rate(decimation_rate)
        if subdev_spec is None:
            subdev_spec = usrp.pick_rx_subdevice(self.usrp)
        self.usrp.set_mux(usrp.determine_rx_mux_value(self.usrp, subdev_spec))
        self.subdev = usrp.selected_subdev(self.usrp, subdev_spec)
        capture_rate = self.usrp.adc_freq() / self.usrp.decim_rate()
        self.info["capture-rate"] = capture_rate
        if gain is None:
            g = self.subdev.gain_range()
            gain = float(g[0]+g[1])/2
        self.subdev.set_gain(gain)
        self.subdev.set_bw(1e6)
        r = self.usrp.tune(0, self.subdev, frequency + self.options.calibration)
        if not r:
            raise RuntimeError("failed to set USRP frequency")
        # capture file
        # if preserve:
        if 0:
            try:
                self.capture_filename = os.tmpnam()
            except RuntimeWarning:
                ignore = True
            capture_file = gr.file_sink(gr.sizeof_gr_complex, self.capture_filename)
            self.__connect([[self.usrp, capture_file]])
        else:
            self.capture_filename = None
        # everything else
        self.__build_graph(self.usrp, capture_rate)

    # Change the UI state
    #
    def _set_state(self, new_state):
        self.state = new_state
        if "STOPPED" == self.state:
            # menu items
            can_capture = self.usrp is not None
            self.file_new.Enable(can_capture)
            self.file_open.Enable(True)
            self.file_properties.Enable(False)
            self.file_close.Enable(False)
            # toolbar
            if self.toolbar:
                self.toolbar.EnableTool(wx.ID_NEW, can_capture)
                self.toolbar.EnableTool(wx.ID_OPEN, True)
            # Visually reflect "no file"
            self.frame.SetStatusText("", 1)
            self.frame.SetStatusText("", 2)
            self.spectrum_plotter.ClearBackground()
            self.signal_plotter.ClearBackground()
            # self.symbol_plotter.ClearBackground()
            # self.traffic.clear()
        elif "RUNNING" == self.state:
            # menu items
            self.file_new.Enable(False)
            self.file_open.Enable(False)
            self.file_properties.Enable(True)
            self.file_close.Enable(True)
            # toolbar
            if self.toolbar:
                self.toolbar.EnableTool(wx.ID_NEW, False)
                self.toolbar.EnableTool(wx.ID_OPEN, False)
        elif "CAPTURING" == self.state:
            # menu items
            self.file_new.Enable(False)
            self.file_open.Enable(False)
            self.file_properties.Enable(True)
            self.file_close.Enable(True)
            # toolbar
            if self.toolbar:
                self.toolbar.EnableTool(wx.ID_NEW, False)
                self.toolbar.EnableTool(wx.ID_OPEN, False)


    # Append filename to default title bar
    #
    def _set_titlebar(self, filename):
        ToDo = True

    # Write capture file properties
    #
    def __write_file_properties(self, filename):
        f = open(filename, "w")
        pickle.dump(self.info, f)
        f.close()

    # Adjust the channel offset
    #
    def adjust_channel_offset(self, delta_hz):
        max_delta_hz = 12000.0
        delta_hz *= self.symbol_deviation      
        delta_hz = max(delta_hz, -max_delta_hz)
        delta_hz = min(delta_hz, max_delta_hz)
        self.channel_filter.set_center_freq(self.channel_offset - delta_hz)

    # Close an open file
    #
    def _on_file_close(self, event):
        self.stop()
        self.wait()
        self.__disconnect()
        if "CAPTURING" == self.state and self.capture_filename:
            dialog = wx.MessageDialog(self.frame, "Save capture file before closing?", style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if wx.ID_YES == dialog.ShowModal():
                save_dialog = wx.FileDialog(self.frame, "Save capture file as:", wildcard="*.dat", style=wx.SAVE|wx.OVERWRITE_PROMPT)
                if save_dialog.ShowModal() == wx.ID_OK:
                    path = str(save_dialog.GetPath())
                    save_dialog.Destroy()
                    os.rename(self.capture_filename, path)
                    self.__write_file_properties(path + ".info")
            else:
                os.remove(self.capture_filename)
        self.capture_filename = None
        self._set_state("STOPPED")

    # New capture from USRP 
    #
    def _on_file_new(self, event):
#         wizard = wx.wizard.Wizard(self.frame, -1, "New Capture from USRP")
#         page1 = wizard_intro_page(wizard)
#         page2 = wizard_details_page(wizard)
#         page3 = wizard_preserve_page(wizard)
#         page4 = wizard_finish_page(wizard)
#         wx.wizard.WizardPageSimple_Chain(page1, page2)
#         wx.wizard.WizardPageSimple_Chain(page2, page3)
#         wx.wizard.WizardPageSimple_Chain(page3, page4)
#         wizard.FitToPage(page1)
#         if wizard.RunWizard(page1):
        self.stop()
        self.wait()
        # ToDo: get open_usrp() arguments from wizard
        self.open_usrp((0,0), 200, None, 434.08e06, True)  # Test freq
        self.start()

    # Open an existing capture
    #
    def _on_file_open(self, event):
        dialog = wx.FileDialog(self.frame, "Choose a capture file:", wildcard="*.dat", style=wx.OPEN)
        if dialog.ShowModal() == wx.ID_OK:
            file = str(dialog.GetPath())
            dialog.Destroy()
            self.stop()
            self.wait()
            self.open_file(file)
            self.start()

    # Present file properties dialog
    #
    def _on_file_properties(self, event):
        # ToDo: show what info we have about the capture file (name,
        # capture source, capture rate, date(?), size(?),)
        todo = True

    # Undo the last edit
    #
    def _on_edit_undo(self, event):
        todo = True

    # Redo the edit
    #
    def _on_edit_redo(self, event):
        todo = True

    # Cut the current selection
    #
    def _on_edit_cut(self, event):
        todo = True

    # Copy the current selection
    #
    def _on_edit_copy(self, event):
        todo = True

    # Paste into the current sample
    #
    def _on_edit_paste(self, event):
        todo = True

    # Delete the current selection
    #
    def _on_edit_delete(self, event):
        todo = True

    # Select all
    #
    def _on_edit_select_all(self, event):
        todo = True

    # Open the preferences dialog
    #
    def _on_edit_prefs(self, event):
        todo = True


    # Set channel offset and RF squelch threshold
    #
    def _on_spectrum_left_click(self, event):
        if "STOPPED" != self.state:
            # set frequency
            x,y = self.spectrum_plotter.GetXY(event)
            xmin, xmax = self.spectrum_plotter.GetXCurrentRange()
            x = min(x, xmax)
            x = max(x, xmin)
            scale_factor = self.spectrum.win._scale_factor
            chan_width = 6.25e3
            x /= scale_factor
            x += chan_width / 2
            x  = (x // chan_width) * chan_width
            self.set_channel_offset(x, scale_factor, self.spectrum.win._units)
            # set squelch threshold
            ymin, ymax = self.spectrum_plotter.GetYCurrentRange()
            y = min(y, ymax)
            y = max(y, ymin)
            squelch_increment = 5
            y += squelch_increment / 2
            y = (y // squelch_increment) * squelch_increment
            self.set_squelch_threshold(int(y))

    # Open an existing capture file
    #
    def open_file(self, capture_file):
        try:
            self.__read_file_properties(capture_file + ".info")
            capture_rate = self.info["capture-rate"]
            self.__set_rx_from_file(capture_file, capture_rate)
            self._set_titlebar(capture_file)
            self._set_state("RUNNING")
        except Exception, x:
            wx.MessageBox("Cannot open capture file: " + x.message, "File Error", wx.CANCEL | wx.ICON_EXCLAMATION)

    def open_audio(self, capture_rate, gain, audio_input_filename):
            self.info = {
                "capture-rate": capture_rate,
                "center-freq": 0,
                "source-dev": "AUDIO",
                "source-decim": 1 }
            self.source = audio.source(capture_rate, audio_input_filename)
            self.__set_rx_from_audio(capture_rate)
            self._set_titlebar("Capturing")
            self._set_state("CAPTURING")

    # Open the USRP
    #
    def open_usrp(self, subdev_spec, decim, gain, frequency, preserve):
        # try:
            self.info = {
                "capture-rate": "unknown",
                "center-freq": frequency,
                "source-dev": "USRP",
                "source-decim": decim }
            self.__set_rx_from_usrp(subdev_spec, decim, gain, frequency, preserve)
            self._set_titlebar("Capturing")
            self._set_state("CAPTURING")
        # except Exception, x:
        #     wx.MessageBox("Cannot open USRP: " + x.message, "USRP Error", wx.CANCEL | wx.ICON_EXCLAMATION)

    # Set the channel offset
    #
    def set_channel_offset(self, offset_hz, scale, units):
        self.channel_offset = -offset_hz
        self.channel_filter.set_center_freq(self.channel_offset)
        self.frame.SetStatusText("Channel offset: " + str(offset_hz * scale) + units, 1)

    # Set the RF squelch threshold level
    #
    def set_squelch_threshold(self, squelch_db):
        self.squelch.set_threshold(squelch_db)
        self.frame.SetStatusText("Squelch: " + str(squelch_db) + "dB", 2)

    def disconnect_demods(self):
# assumes lock held or init
        idx = self.current_speed
        if self.fsk4_demod_connected:
#           self.disconnect(self.mixer, self.lpf, self.arb_resampler, self.resamplers[idx], self.fm_demod, self.baseband_amp, self.symbol_filter, self.fsk4_demod, self.buffer, self.slicer, self.sink_s)
            if self.baseband_input:
                self.disconnect(self.source, self.baseband_amp, self.float_resamplers[self.current_speed], self.symbol_filter)
            else:
                self.disconnect(self.mixer, self.lpf, self.arb_resampler, self.resamplers[idx], self.fm_demod, self.baseband_amp, self.symbol_filter)
            self.disconnect(self.symbol_filter, self.fsk4_demod, self.buffer, self.slicer, self.sink_s)
            self.fsk4_demod_connected = False
        if self.psk_demod_connected:
            self.disconnect(self.mixer, self.lpf, self.arb_resampler, self.resamplers[idx], self.agc, self.symbol_filter_c, self.clock, self.diffdec, self.to_float, self.rescale, self.buffer, self.slicer, self.sink_s)
            self.psk_demod_connected = False

    def connect_psk_demod(self):
# assumes lock held or init
        self.disconnect_demods()
        idx = self.current_speed
        self.connect(self.mixer, self.lpf, self.arb_resampler, self.resamplers[idx], self.agc, self.symbol_filter_c, self.clock, self.diffdec, self.to_float, self.rescale, self.buffer, self.slicer, self.sink_s)
        self.psk_demod_connected = True

    def connect_fsk4_demod(self):
# assumes lock held or init
        self.disconnect_demods()
        idx = self.current_speed
        if self.baseband_input:
            self.connect(self.source, self.baseband_amp, self.float_resamplers[self.current_speed], self.symbol_filter)
        else:
            self.connect(self.mixer, self.lpf, self.arb_resampler, self.resamplers[idx], self.fm_demod, self.baseband_amp, self.symbol_filter)
        self.connect(self.symbol_filter, self.fsk4_demod, self.buffer, self.slicer, self.sink_s)
        self.fsk4_demod_connected = True

    def connect_demods(self):
        if self.baseband_input:
            self.connect_fsk4_demod()
        else:
            if self.fsk4_demod_mode:
                self.connect_fsk4_demod()
            else:
                self.connect_psk_demod()

    def disconnect_data_scope(self):
        if self.data_scope_connected:
            self.disconnect(self.data_scope_input, self.data_scope)
        self.data_scope_connected = False
        self.data_scope_input = None

    def connect_data_scope(self):
        self.disconnect_data_scope()
        sel = self.data_scope.win.radio_box.GetSelection()
        if sel:
            self.data_scope_input = self.symbol_filter
        else:
            self.data_scope_input = self.baseband_amp
        self.data_scope_connected = True
        self.connect(self.data_scope_input, self.data_scope)

    # for datascope, choose monitor viewpoint
    def filter_select(self, evt):
        self.lock()
        self.connect_data_scope()
        self.unlock()

    def corr_select(self, evt):
        new_corr = self.correlation_scope.win.radio_box_corr.GetSelection()
        self.correlation_scope.win.correlation = self.correlation_scope.win.signatures[new_corr]
        if self.baseband_input:
            return
        self.lock()
        self.set_connection()
        if new_corr == len(self.correlation_scope.win.signatures) - 1:
            # special iden mode
            self.corr_i_chan = True
        else:
            self.corr_i_chan = False
        self.set_connection(corr=True)
        self.unlock()

    def speed_select(self, evt):
        new_speed = self.data_scope.win.radio_box_speed.GetSelection()
        print "new_speed %d " % new_speed
        self.lock()
        self.set_speed(new_speed)
        self.unlock()

class window_with_ctlbox(wx.Panel):
    def __init__(self, parent, id = -1):
        wx.Panel.__init__(self, parent, id)

    def make_control_box (self):
        global speeds
        ctrlbox = wx.BoxSizer (wx.HORIZONTAL)

        ctrlbox.Add ((5,0) ,0)

        run_stop = wx.Button (self, 11102, "Run/Stop")
        run_stop.SetToolTipString ("Toggle Run/Stop mode")
        wx.EVT_BUTTON (self, 11102, self.run_stop)
        ctrlbox.Add (run_stop, 0, wx.EXPAND)

        self.radio_box = wx.RadioBox(self, 11103, "Viewpoint", style=wx.RA_SPECIFY_ROWS,
                        choices = ["Raw", "Filtered"] )
        self.radio_box.SetToolTipString("Viewpoint Before Or After Symbol Filter")
        self.radio_box.SetSelection(1)
        ctrlbox.Add (self.radio_box, 0, wx.EXPAND)

        ctrlbox.Add ((5, 0) ,0)            # stretchy space

	speed_str = []
	for speed in speeds:
		speed_str.append("%d" % speed)

        self.radio_box_speed = wx.RadioBox(self, 11104, "Symbol Rate", style=wx.RA_SPECIFY_ROWS, majorDimension=2, choices = speed_str)
        self.radio_box_speed.SetToolTipString("Symbol Rate")
        ctrlbox.Add (self.radio_box_speed, 0, wx.EXPAND)
        ctrlbox.Add ((10, 0) ,1)            # stretchy space

        return ctrlbox

# A snapshot of important fields in current traffic
#
class TrafficPane(wx.Panel):

    # Initializer
    #
    def __init__(self, parent):

        wx.Panel.__init__(self, parent)
        sizer = wx.GridBagSizer(hgap=10, vgap=10)
        self.fields = {}

        label = wx.StaticText(self, -1, "DUID:")
        sizer.Add(label, pos=(1,1))
        field = wx.TextCtrl(self, -1, "", size=(72, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(1,2))
        self.fields["duid"] = field;

        label = wx.StaticText(self, -1, "NAC:")
        sizer.Add(label, pos=(2,1))
        field = wx.TextCtrl(self, -1, "", size=(175, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(2,2))
        self.fields["nac"] = field;

        label = wx.StaticText(self, -1, "Source:")
        sizer.Add(label, pos=(3,1))
        field = wx.TextCtrl(self, -1, "", size=(175, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(3,2))
        self.fields["source"] = field;

        label = wx.StaticText(self, -1, "Destination:")
        sizer.Add(label, pos=(4,1))
        field = wx.TextCtrl(self, -1, "", size=(175, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(4,2))
        self.fields["dest"] = field;

#        label = wx.StaticText(self, -1, "ToDo:")
#        sizer.Add(label, pos=(5,1))
#        field = wx.TextCtrl(self, -1, "", size=(175, -1), style=wx.TE_READONLY)
#        sizer.Add(field, pos=(5,2))
#        self.fields["nid"] = field;

        label = wx.StaticText(self, -1, "MFID:")
        sizer.Add(label, pos=(1,4))
        field = wx.TextCtrl(self, -1, "", size=(175, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(1,5))
        self.fields["mfid"] = field;

        label = wx.StaticText(self, -1, "ALGID:")
        sizer.Add(label, pos=(2,4))
        field = wx.TextCtrl(self, -1, "", size=(175, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(2,5))
        self.fields["algid"] = field;

        label = wx.StaticText(self, -1, "KID:")
        sizer.Add(label, pos=(3,4))
        field = wx.TextCtrl(self, -1, "", size=(72, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(3,5))
        self.fields["kid"] = field;

        label = wx.StaticText(self, -1, "MI:")
        sizer.Add(label, pos=(4,4))
        field = wx.TextCtrl(self, -1, "", size=(216, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(4,5))
        self.fields["mi"] = field;

        label = wx.StaticText(self, -1, "TGID:")
        sizer.Add(label, pos=(5,4))
        field = wx.TextCtrl(self, -1, "", size=(72, -1), style=wx.TE_READONLY)
        sizer.Add(field, pos=(5,5))
        self.fields["tgid"] = field;

        self.SetSizer(sizer)
        self.Fit()

    # Clear the field values
    #
    def clear(self):
        for v in self.fields.values():
            v.Clear()

    # Update the field values
    #
    def update(self, field_values):
        for k,v in self.fields.items():
            f = field_values.get(k, None)
            if f:
                v.SetValue(f)
            else:
                v.SetValue("")


# Introduction page for USRP capture wizard
#
class wizard_intro_page(wx.wizard.WizardPageSimple):

    # Initializer
    #
    def __init__(self, parent):
        wx.wizard.WizardPageSimple.__init__(self, parent)
        html = wx.html.HtmlWindow(self)
        html.SetPage('''
	<html>
	 <body>
         <h1>Capture from USRP</h1>
	 <p>
	  We will guide you through the process of capturing a sample from the USRP.
	  Please ensure that the USRP is switched on and connected to this computer.
	 </p>
	 </body>
	</html>
	''')
        sizer = wx.BoxSizer(wx.VERTICAL)
        self.SetSizer(sizer)
        sizer.Add(html, 1, wx.ALIGN_CENTER | wx.EXPAND | wx.FIXED_MINSIZE)


# USRP wizard details page
#
class wizard_details_page(wx.wizard.WizardPageSimple):

    # Initializer
    #
    def __init__(self, parent):
        wx.wizard.WizardPageSimple.__init__(self, parent)
        sizer = wx.BoxSizer(wx.VERTICAL)
        self.SetSizer(sizer)

    # Return a tuple containing the subdev_spec, gain, frequency, decimation factor
    #
    def get_details(self):
        ToDo = True


# Frequency tracker
#
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


# Decoder watcher
#
class decode_watcher(threading.Thread):

    def __init__(self, msgq, traffic_pane, **kwds):
        threading.Thread.__init__ (self, **kwds)
        self.setDaemon(1)
        self.msgq = msgq
        self.traffic_pane = traffic_pane
        self.keep_running = True
        self.start()

    def run(self):
        while(self.keep_running):
            msg = self.msgq.delete_head()
            pickled_dict = msg.to_string()
            attrs = pickle.loads(pickled_dict)
            self.traffic_pane.update(attrs)

############################################################################
# following code modified from GNURadio sources

default_scopesink_size = (640, 240)
default_v_scale = 1000
default_frame_decim = gr.prefs().get_long('wxgui', 'frame_decim', 1)

class datascope_sink_f(gr.hier_block2):
    def __init__(self, parent, title='', sample_rate=1,
                 size=default_scopesink_size, frame_decim=default_frame_decim,
                 samples_per_symbol=10, num_plots=100,
                 v_scale=default_v_scale, t_scale=None, num_inputs=1, **kwargs):

        gr.hier_block2.__init__(self, "datascope_sink_f",
                                gr.io_signature(num_inputs, num_inputs, gr.sizeof_float),
                                gr.io_signature(0,0,0))

        msgq = gr.msg_queue(2)         # message queue that holds at most 2 messages
        self.st = gr.message_sink(gr.sizeof_float, msgq, dont_block=1)
        self.connect((self, 0), self.st)

        self.win = datascope_window(datascope_win_info (msgq, sample_rate, frame_decim,
                                          v_scale, t_scale, None, title), parent, samples_per_symbol=samples_per_symbol, num_plots=num_plots)

    def set_sample_rate(self, sample_rate):
        self.guts.set_sample_rate(sample_rate)
        self.win.info.set_sample_rate(sample_rate)

# ========================================================================

wxDATA_EVENT = wx.NewEventType()

def EVT_DATA_EVENT(win, func):
    win.Connect(-1, -1, wxDATA_EVENT, func)

class datascope_DataEvent(wx.PyEvent):
    def __init__(self, data):
        wx.PyEvent.__init__(self)
        self.SetEventType (wxDATA_EVENT)
        self.data = data

    def Clone (self): 
        self.__class__ (self.GetId())

class datascope_win_info (object):
    __slots__ = ['msgq', 'sample_rate', 'frame_decim', 'v_scale', 
                 'scopesink', 'title',
                 'time_scale_cursor', 'v_scale_cursor', 'marker', 'xy',
                 'autorange', 'running']

    def __init__ (self, msgq, sample_rate, frame_decim, v_scale, t_scale,
                  scopesink, title = "Oscilloscope", xy=False):
        self.msgq = msgq
        self.sample_rate = sample_rate
        self.frame_decim = frame_decim
        self.scopesink = scopesink
        self.title = title;

        self.marker = 'line'
        self.xy = xy
        self.autorange = not v_scale
        self.running = True

    def set_sample_rate(self, sample_rate):
        self.sample_rate = sample_rate
        
    def get_sample_rate (self):
        return self.sample_rate

    def get_decimation_rate (self):
        return 1.0

    def set_marker (self, s):
        self.marker = s

    def get_marker (self):
        return self.marker


class datascope_input_watcher (threading.Thread):
    def __init__ (self, msgq, event_receiver, frame_decim, num_plots, samples_per_symbol, **kwds):
        threading.Thread.__init__ (self, **kwds)
        self.setDaemon (1)
        self.msgq = msgq
        self.event_receiver = event_receiver
        self.frame_decim = frame_decim
        self.samples_per_symbol = samples_per_symbol
        self.num_plots = num_plots
        self.iscan = 0
        self.keep_running = True
        self.skip = 0
        self.totsamp = 0
        self.skip_samples = 0
        self.start ()
        self.msg_string = ""

    def run (self):
        # print "datascope_input_watcher: pid = ", os.getpid ()
        while (self.keep_running):
            msg = self.msgq.delete_head()   # blocking read of message queue
            nchan = int(msg.arg1())    # number of channels of data in msg
            nsamples = int(msg.arg2()) # number of samples in each channel
            self.totsamp += nsamples
            if self.skip_samples >= nsamples:
               self.skip_samples -= nsamples
               continue

            self.msg_string += msg.to_string()      # body of the msg as a string

            bytes_needed = (self.num_plots*self.samples_per_symbol) * gr.sizeof_float
            if (len(self.msg_string) < bytes_needed):
                continue

            records = []
            # start = self.skip * gr.sizeof_float
            start = 0
            chan_data = self.msg_string[start:start+bytes_needed]
            rec = numpy.fromstring (chan_data, numpy.float32)
            records.append (rec)
            self.msg_string = ""

            unused = nsamples - (self.num_plots*self.samples_per_symbol)
            unused -= (start/gr.sizeof_float)
            self.skip = self.samples_per_symbol - (unused % self.samples_per_symbol)
            # print "reclen = %d totsamp %d appended %d skip %d start %d unused %d" % (nsamples, self.totsamp, len(rec), self.skip, start/gr.sizeof_float, unused)

            de = datascope_DataEvent (records)
            wx.PostEvent (self.event_receiver, de)
            records = []
            del de

            # lower values = more frequent plots, but higher CPU usage
            self.skip_samples = self.num_plots * self.samples_per_symbol * 20

class datascope_window (window_with_ctlbox):

    def __init__ (self, info, parent, id = -1,
                  samples_per_symbol=10, num_plots=100,
                  pos = wx.DefaultPosition, size = wx.DefaultSize, name = ""):
        window_with_ctlbox.__init__ (self, parent, -1)
        self.info = info

        vbox = wx.BoxSizer (wx.VERTICAL)

        self.graph = datascope_graph_window (info, self, -1, samples_per_symbol=samples_per_symbol, num_plots=num_plots)

        vbox.Add (self.graph, 1, wx.EXPAND)
        vbox.Add (self.make_control_box(), 0, wx.EXPAND)
        vbox.Add (self.make_control2_box(), 0, wx.EXPAND)

        self.sizer = vbox
        self.SetSizer (self.sizer)
        self.SetAutoLayout (True)
        self.sizer.Fit (self)
        

    # second row of control buttons etc. appears BELOW control_box
    def make_control2_box (self):
        ctrlbox = wx.BoxSizer (wx.HORIZONTAL)

        ctrlbox.Add ((5,0) ,0) # left margin space

        return ctrlbox
    
    def run_stop (self, evt):
        self.info.running = not self.info.running

class datascope_graph_window (plot.PlotCanvas):

    def __init__ (self, info, parent, id = -1,
                  pos = wx.DefaultPosition, size = (140, 140),
                  samples_per_symbol=10, num_plots=100,
                  style = wx.DEFAULT_FRAME_STYLE, name = ""):
        plot.PlotCanvas.__init__ (self, parent, id, pos, size, style, name)

        self.SetXUseScopeTicks (True)
        self.SetEnableGrid (False)
        self.SetEnableZoom (True)
        self.SetEnableLegend(True)
        # self.SetBackgroundColour ('black')
        
        self.info = info;

        self.total_points = 0

        self.samples_per_symbol = samples_per_symbol
        self.num_plots = num_plots

        EVT_DATA_EVENT (self, self.format_data)

        self.input_watcher = datascope_input_watcher (info.msgq, self, info.frame_decim, self.samples_per_symbol, self.num_plots)

    def format_data (self, evt):
        if not self.info.running:
            return
        
        info = self.info
        records = evt.data
        nchannels = len (records)
        npoints = len (records[0])
        self.total_points += npoints

        x_vals = numpy.arange (0, self.samples_per_symbol)

        self.SetXUseScopeTicks (True)   # use 10 divisions, no labels

        objects = []
        colors = ['red','orange','yellow','green','blue','violet','cyan','magenta','brown','black']

        r = records[0]  # input data
        for i in range(self.num_plots):
            points = []
            for j in range(self.samples_per_symbol):
                p = [ j, r[ i*self.samples_per_symbol + j ] ]
                points.append(p)
            objects.append (plot.PolyLine (points, colour=colors[i % len(colors)], legend=('')))

        graphics = plot.PlotGraphics (objects,
                                      title='Data Scope',
                                      xLabel = 'Time', yLabel = 'Amplitude')

        x_range = (0., 0. + (self.samples_per_symbol-1)) # ranges are tuples!
        self.y_range = (-4., 4.) # for standard -3/-1/+1/+3
        # self.y_range = (-10., 10.) # for standard -3/-1/+1/+3
        self.Draw (graphics, xAxis=x_range, yAxis=self.y_range)
############################################################################
class constellation_plot_c(gr.hier_block2):
    def __init__(self, parent, title='', sample_rate=1,
                 frame_decim=10,
                 num_plots=100,
                 num_inputs=1, **kwargs):

        gr.hier_block2.__init__(self, "constellation_plot_c",
                                gr.io_signature(num_inputs, num_inputs, gr.sizeof_gr_complex),
                                gr.io_signature(0,0,0))

        msgq = gr.msg_queue(2)         # message queue that holds at most 2 messages
        self.st = gr.message_sink(gr.sizeof_gr_complex, msgq, dont_block=1)
        self.connect((self, 0), self.st)

        self.win = constellation_plot_window(constellation_plot_win_info (msgq, sample_rate, frame_decim, None, title), parent, num_plots=num_plots)

    def set_sample_rate(self, sample_rate):
        self.guts.set_sample_rate(sample_rate)
        self.win.info.set_sample_rate(sample_rate)

# ========================================================================

wxDATA_EVENT = wx.NewEventType()

def EVT_DATA_EVENT(win, func):
    win.Connect(-1, -1, wxDATA_EVENT, func)

class constellation_plot_DataEvent(wx.PyEvent):
    def __init__(self, data):
        wx.PyEvent.__init__(self)
        self.SetEventType (wxDATA_EVENT)
        self.data = data

    def Clone (self): 
        self.__class__ (self.GetId())

class constellation_plot_win_info (object):
    __slots__ = ['msgq', 'sample_rate', 'frame_decim',
                 'scopesink', 'title',
                 'time_scale_cursor', 'marker', 'xy',
                 'autorange', 'running']

    def __init__ (self, msgq, sample_rate, frame_decim,
                  scopesink, title = "Oscilloscope", xy=True):
        self.msgq = msgq
        self.sample_rate = sample_rate
        self.frame_decim = frame_decim
        self.scopesink = scopesink
        self.title = title;

        self.marker = 'line'
        self.xy = xy
        self.autorange = False
        self.running = True

    def set_sample_rate(self, sample_rate):
        self.sample_rate = sample_rate
        
    def get_sample_rate (self):
        return self.sample_rate

    def get_decimation_rate (self):
        return 1.0

    def set_marker (self, s):
        self.marker = s

    def get_marker (self):
        return self.marker


class constellation_plot_input_watcher (threading.Thread):
    def __init__ (self, msgq, event_receiver, frame_decim, num_plots, **kwds):
        threading.Thread.__init__ (self, **kwds)
        self.setDaemon (1)
        self.msgq = msgq
        self.event_receiver = event_receiver
        self.frame_decim = frame_decim
        self.num_plots = num_plots
        self.iscan = 0
        self.keep_running = True
        self.skip = 0
        self.totsamp = 0
        self.skip_samples = 0
        self.start ()
        self.msg_string = ""
        self.skip_mode = False

    def run (self):
        # print "constellation_plot_input_watcher: pid = ", os.getpid ()
        time.sleep(1)
        while (self.keep_running):
            bytes_needed = self.num_plots * gr.sizeof_float * 2
            if self.skip_mode:
                bytes_needed = 500 * gr.sizeof_float * 2

            if len(self.msg_string) < bytes_needed:
                msg = self.msgq.delete_head()   # blocking read of message queue
                nchan = int(msg.arg1())    # number of channels of data in msg
                nsamples = int(msg.arg2()) # number of samples in each channel
                self.totsamp += nsamples

                self.msg_string += msg.to_string()      # body of the msg as a string
                continue

            chan_data = self.msg_string[:bytes_needed]
            self.msg_string = self.msg_string[bytes_needed:]

            if self.skip_mode:
                self.skip_mode = False
                continue

            records = []
            # start = self.skip * gr.sizeof_gr_complex
            # start = 0
            # chan_data = self.msg_string[start:start+bytes_needed]
            rec = numpy.fromstring (chan_data, numpy.float32)
            records.append (rec)
            # self.msg_string = ""

            # unused = nsamples - self.num_plots
            # unused -= (start/gr.sizeof_gr_complex)
            # print "reclen = %d totsamp %d appended %d skip %d start %d unused %d" % (nsamples, self.totsamp, len(rec), self.skip, start/gr.sizeof_float, unused)

            de = constellation_plot_DataEvent (records)
            wx.PostEvent (self.event_receiver, de)
            records = []
            del de

            # lower values = more frequent plots, but higher CPU usage
            # self.skip_samples = 5000
            self.skip_mode = True

class constellation_plot_window (wx.Panel):

    def __init__ (self, info, parent, id = -1,
                  num_plots=100,
                  pos = wx.DefaultPosition, size = wx.DefaultSize, name = ""):
        wx.Panel.__init__ (self, parent, -1)
        self.info = info

        vbox = wx.BoxSizer (wx.VERTICAL)

        self.graph = constellation_plot_graph_window (info, self, -1, num_plots=num_plots)

        vbox.Add (self.graph, 1, wx.EXPAND)
        vbox.Add (self.make_control_box(), 0, wx.EXPAND)
        vbox.Add (self.make_control2_box(), 0, wx.EXPAND)

        self.sizer = vbox
        self.SetSizer (self.sizer)
        self.SetAutoLayout (True)
        self.sizer.Fit (self)
        

    # second row of control buttons etc. appears BELOW control_box
    def make_control2_box (self):
        ctrlbox = wx.BoxSizer (wx.HORIZONTAL)

        ctrlbox.Add ((5,0) ,0) # left margin space

        return ctrlbox

    def make_control_box (self):
        ctrlbox = wx.BoxSizer (wx.HORIZONTAL)

        ctrlbox.Add ((5,0) ,0)

        run_stop = wx.Button (self, 11102, "Run/Stop")
        run_stop.SetToolTipString ("Toggle Run/Stop mode")
        wx.EVT_BUTTON (self, 11102, self.run_stop)
        ctrlbox.Add (run_stop, 0, wx.EXPAND)

        self.radio_box = wx.RadioBox(self, 11103, "Viewpoint", style=wx.RA_SPECIFY_ROWS,
                        choices = ["Raw", "Filtered"] )
        self.radio_box.SetToolTipString("Viewpoint Before Or After Symbol Filter")
        ctrlbox.Add (self.radio_box, 0, wx.EXPAND)

        ctrlbox.Add ((10, 0) ,1)            # stretchy space

        return ctrlbox
    
    def run_stop (self, evt):
        self.info.running = not self.info.running

class constellation_plot_graph_window (plot.PlotCanvas):

    def __init__ (self, info, parent, id = -1,
                  pos = wx.DefaultPosition, size = (140, 140),
                  num_plots=100,
                  style = wx.DEFAULT_FRAME_STYLE, name = ""):
        plot.PlotCanvas.__init__ (self, parent, id, pos, size, style, name)

        self.SetXUseScopeTicks (True)
        self.SetEnableGrid (False)
        self.SetEnableZoom (True)
        self.SetEnableLegend(True)
        # self.SetBackgroundColour ('black')
        
        self.info = info;

        self.total_points = 0

        self.num_plots = num_plots

        EVT_DATA_EVENT (self, self.format_data)

        self.input_watcher = constellation_plot_input_watcher (info.msgq, self, info.frame_decim, self.num_plots)

        self.flag = False

    def format_data (self, evt):
        if not self.info.running:
            return

        info = self.info
        records = evt.data
        nchannels = len (records)
        npoints = len (records[0])
        self.total_points += npoints

        self.SetXUseScopeTicks (True)   # use 10 divisions, no labels

        objects = []

        r = records[0]  # input data
        l = len(r) / 2
        p0 = []
        p1 = []
        for i in range(l):
            p = [ r[ i*2 ], r[ i*2+1 ] ]
            if self.flag:
                p1.append(p)
            else:
                p0.append(p)
            self.flag = not self.flag

        objects.append (plot.PolyMarker (p0, marker='plus', colour='blue'))
        objects.append (plot.PolyMarker (p1, marker='plus', colour='blue'))

        graphics = plot.PlotGraphics (objects,
                                      title='Data Scope',
                                      xLabel = 'I', yLabel = 'Q')

        x_range = (-1.0, 1.0)
        y_range = (-1.0, 1.0)
        self.Draw (graphics, xAxis=x_range, yAxis=y_range)

    def format_data_pop (self, evt):
        if not self.info.running:
            return

        info = self.info
        records = evt.data
        nchannels = len (records)
        npoints = len (records[0])
        self.total_points += npoints

        self.SetXUseScopeTicks (True)   # use 10 divisions, no labels

        objects = []
        colors = ['red','orange','yellow','green','blue','violet','cyan','magenta','brown','black']

        r = records[0]  # input data
        l = len(r) / 2
        b0 = []
        b1 = []
        max_buckets = 6.0
        m = int(2 * pi * max_buckets)
        for i in range(m+1):
            b0.append(0)
            b1.append(0)
        for i in range(l):
            # p = [ r[ i*2 ], r[ i*2+1 ] ]
            # if self.flag:
            #     p1.append(p)
            # else:
            #     p0.append(p)
            theta = math.atan2 ( r[ i*2 ], r[ i*2+1 ] )
            bucket = int((theta + pi) * max_buckets)
            if 1:
                if self.flag:
                    b0[bucket] += 1
                else:
                    b1[bucket] += 1
            self.flag = not self.flag

        # determine avg. "power" - for later rescaling of the values 
        tot = ct = 0
        for b in b0+b1:
            tot += b
            ct += 1
        avg = float(tot) / float(ct)

        p0 = []
        p1 = []
        r = len(b0)
        for i in range(r):
            theta = ((float(i)/ r) * 2 * pi) - pi
            abs = 0.5 * b0[i] / avg
            p = [ abs * math.cos(theta), abs * math.sin(theta) ]
            if i == 0:
                sp = p
            p0.append(p)
        p0.append(sp)
        r = len(b1)
        for i in range(r):
            theta = ((float(i) / r) * 2 * pi) - pi
            abs = 0.5 * b1[i] / avg
            p = [ abs * math.cos(theta), abs * math.sin(theta) ]
            if i == 0:
                sp = p
            p1.append(p)
        p1.append(sp)
        # objects.append (plot.PolyMarker (p0, marker='point', colour='red'))
        # objects.append (plot.PolyMarker (p1, marker='point', colour='green'))
        objects.append (plot.PolyLine (p0, colour='red', legend=''))
        objects.append (plot.PolyLine (p1, colour='green', legend=''))

        graphics = plot.PlotGraphics (objects,
                                      title='Data Scope',
                                      xLabel = 'I', yLabel = 'Q')

        x_range = (-2.5, 2.5)
        y_range = (-2.5, 2.5)
        self.Draw (graphics, xAxis=x_range, yAxis=y_range)
############################################################################
class correlation_plot_f(gr.hier_block2):
    def __init__(self, parent, title='', sps=10,
                 frame_decim=4,
                 num_inputs=1, **kwargs):

        gr.hier_block2.__init__(self, "correlation_plot_f",
                                gr.io_signature(num_inputs, num_inputs, gr.sizeof_float),
                                gr.io_signature(0,0,0))

        msgq = gr.msg_queue(2)         # message queue that holds at most 2 messages
        self.st = gr.message_sink(gr.sizeof_float, msgq, dont_block=1)
        self.connect((self, 0), self.st)

        self.win = correlation_plot_window(correlation_plot_win_info (msgq, sps, frame_decim, None, title), parent, sps=sps)

# ========================================================================

wxDATA_EVENT = wx.NewEventType()

def EVT_DATA_EVENT(win, func):
    win.Connect(-1, -1, wxDATA_EVENT, func)

class correlation_plot_DataEvent(wx.PyEvent):
    def __init__(self, data):
        wx.PyEvent.__init__(self)
        self.SetEventType (wxDATA_EVENT)
        self.data = data

    def Clone (self): 
        self.__class__ (self.GetId())

class correlation_plot_win_info (object):
    __slots__ = ['msgq', 'sps', 'frame_decim',
                 'scopesink', 'title',
                 'time_scale_cursor', 'marker', 'xy',
                 'autorange', 'running']

    def __init__ (self, msgq, sps, frame_decim,
                  scopesink, title = "Oscilloscope", xy=True):
        self.msgq = msgq
        self.sps = sps
        self.frame_decim = frame_decim
        self.scopesink = scopesink
        self.title = title;

        self.marker = 'line'
        self.xy = xy
        self.autorange = False
        self.running = True

    def get_decimation_rate (self):
        return 1.0

    def set_marker (self, s):
        self.marker = s

    def get_marker (self):
        return self.marker

class correlation_plot_input_watcher (threading.Thread):
    def __init__ (self, msgq, event_receiver, frame_decim, **kwds):
        threading.Thread.__init__ (self, **kwds)
        self.setDaemon (1)
        self.msgq = msgq
        self.event_receiver = event_receiver
        self.frame_decim = frame_decim
        self.iscan = 0
        self.keep_running = True
        self.skip = 0
        self.totsamp = 0
        self.skip_samples = 0
        self.start ()
        self.msg_string = ""
        self.skip_mode = False

    def run (self):
        # print "correlation_plot_input_watcher: pid = ", os.getpid ()
        time.sleep(1)
        while (self.keep_running):
            bytes_needed = 24000 * gr.sizeof_float

            if len(self.msg_string) < bytes_needed:
                msg = self.msgq.delete_head()   # blocking read of message queue
                nchan = int(msg.arg1())    # number of channels of data in msg
                nsamples = int(msg.arg2()) # number of samples in each channel
                self.totsamp += nsamples

                self.msg_string += msg.to_string()      # body of the msg as a string
                continue

            chan_data = self.msg_string[:bytes_needed]
            self.msg_string = self.msg_string[bytes_needed:]

#           if self.skip_mode:
#               self.skip_mode = False
#               continue

            records = []
            # start = self.skip * gr.sizeof_gr_complex
            # start = 0
            # chan_data = self.msg_string[start:start+bytes_needed]
            rec = numpy.fromstring (chan_data, numpy.float32)
            records.append (rec)
            # self.msg_string = ""

            # unused = nsamples - self.num_plots
            # unused -= (start/gr.sizeof_gr_complex)
            # print "reclen = %d totsamp %d appended %d skip %d start %d unused %d" % (nsamples, self.totsamp, len(rec), self.skip, start/gr.sizeof_float, unused)

            de = correlation_plot_DataEvent (records)
            wx.PostEvent (self.event_receiver, de)
            records = []
            del de

            # lower values = more frequent plots, but higher CPU usage
            # self.skip_samples = 5000
#           self.skip_mode = True

class correlation_plot_window (wx.Panel):

    def __init__ (self, info, parent, id = -1,
                  sps=10,
                  pos = wx.DefaultPosition, size = wx.DefaultSize, name = ""):
        wx.Panel.__init__ (self, parent, -1)
        self.info = info

        vbox = wx.BoxSizer (wx.HORIZONTAL)

        self.graph = correlation_plot_graph_window (info, self, -1, sps=sps)

        vbox.Add (self.graph, 1, wx.EXPAND)
        vbox.Add (self.make_control_box(), 0, wx.EXPAND)
#       vbox.Add (self.make_control2_box(), 0, wx.EXPAND)

        self.sizer = vbox
        self.SetSizer (self.sizer)
        self.SetAutoLayout (True)
        self.sizer.Fit (self)
        

    # second row of control buttons etc. appears BELOW control_box
    def make_control2_box (self):
        ctrlbox = wx.BoxSizer (wx.HORIZONTAL)

        ctrlbox.Add ((5,0) ,0) # left margin space

        return ctrlbox

    def make_control_box (self):
        # 48k iden sync sig
        iden_frame_sync = [0.131053, 0.762875, 0.985880, 0.692932, 0.021247, -0.509172, -0.436476, 0.121728, 0.574703, 0.545912, 0.008813, -0.676659, -0.920639, -0.490609, 0.182287, 0.632788, 0.737212, 0.681760, 0.737237, 0.937172, 1.009479, 0.794382, 0.339788, 0.026356, 0.178487, 0.627079, 0.902744, 0.742624, 0.165377, -0.442614, -0.691702, -0.454418, -0.135002]
        ctrlbox = wx.BoxSizer (wx.HORIZONTAL)

        ctrlbox.Add ((5,0) ,0)

        # read directory of correlation signatures
        ents = []
        self.signatures = []
        r = re.compile(r'^[13]+$')
        path = "corr"
        for fn in os.listdir(path):
            if not r.match(fn):
                continue
            f = open("%s/%s" % (path, fn))
            line = f.readline()
            f.close()
            ents.append(line.strip())

            frame_sync = []
            for c in fn:
                if c == '1':
                    frame_sync.append(1)
                else:	# 3
                    frame_sync.append(-1)
	    correlation = []
            for symbol in frame_sync:
                for i in xrange(self.info.sps):
                    correlation.append(symbol)
            correlation.reverse()	# reverse order for convolve()
            self.signatures.append(correlation)

        #special final entry for iden
        ents.append('iDEN')
        correlation = iden_frame_sync
        correlation.reverse()	# reverse order for convolve()
        self.signatures.append(correlation)

        self.radio_box_corr = wx.RadioBox(self, 11105, "Sync Signature", style=wx.RA_SPECIFY_COLS,
                        majorDimension=2, choices = ents )
        self.radio_box_corr.SetToolTipString("Signatures of Known Signal Types")

        ctrlbox.Add (self.radio_box_corr, 0, wx.EXPAND)

        ctrlbox.Add ((10, 0) ,1)            # stretchy space

        return ctrlbox
    
    def run_stop (self, evt):
        self.info.running = not self.info.running

class correlation_plot_graph_window (plot.PlotCanvas):

    def __init__ (self, info, parent, id = -1,
                  pos = wx.DefaultPosition, size = (140, 140),
                  sps=10,
                  style = wx.DEFAULT_FRAME_STYLE, name = ""):
        plot.PlotCanvas.__init__ (self, parent, id, pos, size, style, name)

        self.SetXUseScopeTicks (True)
        self.SetEnableGrid (False)
        self.SetEnableZoom (True)
        self.SetEnableLegend(True)
        # self.SetBackgroundColour ('black')
#        self.Zoom([0, 0], [1.0, 1.0])
        
        self.info = info;
        self.parent = parent;

        self.total_points = 0

        EVT_DATA_EVENT (self, self.format_data)

        self.input_watcher = correlation_plot_input_watcher (info.msgq, self, info.frame_decim)

    def format_data (self, evt):
        if not self.info.running:
            return

        info = self.info
        records = evt.data
        nchannels = len (records)
        npoints = len (records[0])
        self.total_points += npoints

        self.SetXUseScopeTicks (True)   # use 10 divisions, no labels

        objects = []

        r = records[0]  # input data

        sig = self.parent.signatures[self.parent.radio_box_corr.GetSelection()]
	res = numpy.convolve(r, sig, mode='valid')
        p0 = []
        i = 0
        for p in res:
            p0.append([i, p])
            i += 1

        objects.append (plot.PolyLine (p0, colour='blue'))

        graphics = plot.PlotGraphics (objects,
                                      title='Correlation',
                                      xLabel = '', yLabel = '')

        x_range = (0, len(res))
        y_range = (-800.0, 800.0)
        self.Draw (graphics, xAxis=x_range, yAxis=y_range)

############################################################################

# Start the receiver
#
if '__main__' == __name__:
    app = stdgui2.stdapp(p25_rx_block, "APCO P25 Receiver", 3)
    app.MainLoop()
