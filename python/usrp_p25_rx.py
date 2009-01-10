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

import pickle
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

# The P25 receiver
#
class p25_rx_block (stdgui2.std_top_block):

    # Initialize the P25 receiver
    #
    def __init__(self, frame, panel, vbox, argv):
        stdgui2.std_top_block.__init__(self, frame, panel, vbox, argv)

        # setup (read-only) attributes
        self.channel_rate = 125000
        self.symbol_rate = 4800
        self.symbol_deviation = 600.0

        # keep track of flow graph connections
        self.cnxns = []

        # initialize the UI
        # 
        self.__init_gui(frame, panel, vbox)

        # command line argument parsing
        #
        parser = OptionParser(option_class=eng_option)
        parser.add_option("-i", "--input", default=None, help="input file name")
        parser.add_option("-f", "--frequency", type="eng_float", default=0.0, help="USRP center frequency", metavar="Hz")
        parser.add_option("-d", "--decim", type="int", default=256, help="source decimation factor")
        parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=(0, 0), help="select USRP Rx side A or B (default=A)")
        parser.add_option("-g", "--gain", type="eng_float", default=None, help="set USRP gain in dB (default is midpoint)")
        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)

        # configure specified data source
        #
        if options.input:
            self.open_file(options.input)
        elif options.frequency:
            from gnuradio import usrp
            self._set_state("CAPTURING")
            self.__set_rx_from_usrp(options.decim, options.subdev_spec, options.gain, options.frequency)
        else:
            self._set_state("STOPPED")

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

    # initialize the UI
    # 
    def __init_gui(self, frame, panel, vbox):
        self.frame = frame
        self.frame.CreateStatusBar()
        self.panel = panel
        self.vbox = vbox
        
        # setup the "File" menu
        menubar = self.frame.GetMenuBar()
        file_menu = menubar.GetMenu(0)
        self.file_capture = file_menu.Insert(0, wx.ID_NEW, u'&New Capture...\tCtrl-C', 'Capture from USRP', wx.ITEM_NORMAL)
        self.frame.Bind(wx.EVT_MENU, self._on_file_capture, self.file_capture)
#        self.file_open = file_menu.Insert(1, wx.ID_OPEN, u'&Open...\tCtrl-O', 'Open file', wx.ITEM_NORMAL)
        self.file_open = file_menu.Insert(1, wx.ID_OPEN)
        self.frame.Bind(wx.EVT_MENU, self._on_file_open, self.file_open)
        file_menu.InsertSeparator(2)
        self.file_properties = file_menu.Insert(3, wx.ID_PROPERTIES, u'Properties...\tAlt-Enter', 'File properties.', wx.ITEM_NORMAL)
        self.frame.Bind(wx.EVT_MENU, self._on_file_properties, self.file_properties)
        file_menu.InsertSeparator(4)
        self.file_close = file_menu.Insert(5, wx.ID_CLOSE, u'Close\tCtrl-W', 'Close file', wx.ITEM_NORMAL)
        self.frame.Bind(wx.EVT_MENU, self._on_file_close, self.file_close)

        # setup the toolbar
        if False:
            toolbar = wx.ToolBar(frame, -1, style = wx.TB_DOCKABLE | wx.TB_HORIZONTAL)
            frame.SetToolBar(toolbar)
            icon_size = wx.Size(24, 24)
            new_icon = wx.ArtProvider.GetBitmap(wx.ART_NEW, wx.ART_TOOLBAR, icon_size)
            self.toolbar_capture = toolbar.AddSimpleTool(wx.ID_NEW, new_icon, u"New Capture")
            open_icon = wx.ArtProvider.GetBitmap(wx.ART_FILE_OPEN, wx.ART_TOOLBAR, icon_size)
            self.toolbar_open = toolbar.AddSimpleTool(wx.ID_OPEN, open_icon, u"Open")

        # setup the notebook
        self.notebook = wx.Notebook(self.panel)
        self.vbox.Add(self.notebook, 1, wx.EXPAND)       
        # add spectrum scope
        self.spectrum = fftsink2.fft_sink_c(self.notebook, fft_size=512, average=True, peak_hold=True)
        self.spectrum_plotter = self.spectrum.win.plot
        self.spectrum_plotter.Bind(wx.EVT_LEFT_DOWN, self._on_spectrum_left_click)
        self.notebook.AddPage(self.spectrum.win, "RF Spectrum")
        # add C4FM scope
        self.signal_scope = scopesink2.scope_sink_f(self.notebook, sample_rate = self.channel_rate, v_scale=5, t_scale=0.001)
        self.signal_plotter = self.signal_scope.win.graph
        self.notebook.AddPage(self.signal_scope.win, "C4FM Signal")
        # add symbol scope
        self.symbol_scope = scopesink2.scope_sink_f(self.notebook, frame_decim=1, sample_rate=self.symbol_rate, v_scale=1, t_scale=0.05)
        self.symbol_plotter = self.symbol_scope.win.graph
        self.symbol_scope.win.set_format_plus()
        self.notebook.AddPage(self.symbol_scope.win, "Demodulated Symbols")
#         # ToDo: add info tab
#         # Report the TUN/TAP device name
        self.p25_decoder = op25.decoder_ff()
        self.frame.SetStatusText("TUN/TAP: " + self.p25_decoder.device_name())

    # read capture file properties (decimation etc.)
    #
    def __read_file_properties(self, filename):
        f = open(filename, "r")
        self.info = pickle.load(f)
        f.close()

    # setup to rx from USRP
    #
    def __set_rx_from_usrp(self, decimation_rate, subdev_spec, gain, frequency):
        # USRP
        u = usrp.source_c(decim_rate=decimation_rate)
        if subdev_spec is None:
            subdev_spec = usrp.pick_rx_subdevice(u)
        u.set_mux(usrp.determine_rx_mux_value(u, subdev_spec))
        subdev = usrp.selected_subdev(u, subdev_spec)
        #print "Using RX d'board %s" % (subdev.side_and_name(),)
        capture_rate = u.adc_freq() / u.decim_rate()
        #print "USB sample rate %s" % (eng_notation.num_to_str(capture_rate))
        if gain is None:
            # if no gain was specified, use the mid-point in dB
            g = subdev.gain_range()
            gain = float(g[0]+g[1])/2
        subdev.set_gain(gain)
        r = u.tune(0, subdev, frequency)
        if not r:
            raise SystemExit, "Failed to set USRP frequency"
        self._build_graph(u, capture_rate)

    # setup to rx from file
    #
    def __set_rx_from_file(self, filename, capture_rate):
        # input file
        file = gr.file_source(gr.sizeof_gr_complex, filename, True)
        # throttle to source rate
        throttle = gr.throttle(gr.sizeof_gr_complex, capture_rate)
        self.__connect([[file, throttle]])
        self._build_graph(throttle, capture_rate)

    # setup common flow graph elements
    #
    def _build_graph(self, source, capture_rate):
        # tell the scope the source rate
        self.spectrum.set_sample_rate(capture_rate)
        # channel filter
        self.channel_offset = 0.0
        channel_decim = capture_rate // self.channel_rate
        channel_rate = capture_rate // channel_decim
        trans_width = 12.5e3 / 2;
        trans_centre = trans_width + (trans_width / 2)
        coeffs = gr.firdes.low_pass(1.0, capture_rate, trans_centre, trans_width, gr.firdes.WIN_HANN)
        self.channel_filter = gr.freq_xlating_fir_filter_ccf(channel_decim, coeffs, 0.0, capture_rate)
        self.set_channel_offset(0.0, 0, self.spectrum.win._units)
        # power squelch
        squelch_db = 0
        self.squelch = gr.pwr_squelch_cc(squelch_db, 1e-3, 0, True)
        self.set_squelch_threshold(squelch_db)
        # FM demodulator
        fm_demod_gain = channel_rate / (2.0 * pi * self.symbol_deviation)
        fm_demod = gr.quadrature_demod_cf(fm_demod_gain)
        # symbol filter        
        symbol_decim = 1
        symbol_coeffs = gr.firdes.root_raised_cosine(1.0, channel_rate, self.symbol_rate, 0.2, 500)
        symbol_filter = gr.fir_filter_fff(symbol_decim, symbol_coeffs)
        # C4FM demodulator
        autotuneq = gr.msg_queue(2)
        self.demod_watcher = demod_watcher(autotuneq, self.adjust_channel_offset)
        demod_fsk4 = fsk4.demod_ff(autotuneq, channel_rate, self.symbol_rate)
        # for now no audio output
        sink = gr.null_sink(gr.sizeof_float)
        # connect it all up
        self.__connect([[source, self.channel_filter, self.squelch, fm_demod, symbol_filter, demod_fsk4, self.p25_decoder, sink],
                      [source, self.spectrum],
                      [symbol_filter, self.signal_scope],
                      [demod_fsk4, self.symbol_scope]])

    # Change the UI state
    #
    def _set_state(self, new_state):
        self.state = new_state
        if "STOPPED" == self.state:
            self.file_capture.Enable(True)
#            self.toolbar_capture.Enable(True)
            self.file_open.Enable(True)
#            self.toolbar_open.Enable(True)
            self.file_properties.Enable(False)
            self.file_close.Enable(False)
            # Visually reflect "no file"
            self.frame.SetStatusText("", 1)
            self.frame.SetStatusText("", 2)
            self.spectrum_plotter.Clear()
            self.signal_plotter.Clear()
            self.symbol_plotter.Clear()
        elif "RUNNING" == self.state:
            self.file_capture.Enable(False)
#            self.toolbar_capture.Enable(False)
            self.file_open.Enable(False)
#            self.toolbar_open.Enable(False)
            self.file_properties.Enable(True)
            self.file_close.Enable(True)
        elif "CAPTURING" == self.state:
            self.file_capture.Enable(False)
#            self.toolbar_capture.Enable(False)
            self.file_open.Enable(False)
#            self.toolbar_open.Enable(False)
            self.file_properties.Enable(True)
            self.file_close.Enable(True)

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

    # Coarse frequency tracker
    #
    def adjust_channel_offset(self, delta_hz):
        max_delta_hz = 12000.0
        delta_hz *= self.symbol_deviation      
        delta_hz = max(delta_hz, -max_delta_hz)
        delta_hz = min(delta_hz, max_delta_hz)
        self.channel_filter.set_center_freq(self.channel_offset - delta_hz)

    # New capture from USRP 
    #
    def _on_file_capture(self, event):
        wizard = wx.wizard.Wizard(None, -1, "New Capture from USRP")
        # ToDo: set up proper wizard pages
        page1 = wx.wizard.WizardPageSimple(wizard) # Intro - "please ensure you have plugged it in etc.", link to webpage
        page2 = wx.wizard.WizardPageSimple(wizard) # Device/Card/Rate (implies decim) - all comboboxes 
        page3 = wx.wizard.WizardPageSimple(wizard) # Capture data to disk? Explain why decision needed now.
        page4 = wx.wizard.WizardPageSimple(wizard) # You are good to go...
        wx.wizard.WizardPageSimple_Chain(page1, page2)
        wx.wizard.WizardPageSimple_Chain(page2, page3)
        wx.wizard.WizardPageSimple_Chain(page3, page4)
        wizard.FitToPage(page1)
        if wizard.RunWizard(page1):
            self._set_titlebar("Unsaved capture file")
            # ToDo: scrape data,
            # ToDo: __set_rx_from_usrp(scraped data)
            # self.start()
            self._set_state("CAPTURING")

    # Close an open file
    #
    def _on_file_close(self, event):
        self.stop()
        self.wait()
        self.__disconnect()
        # ToDo: check if capture has been logged to disk
        if "CAPTURING" == self.state:
            dialog = wx.MessageDialog(self.frame, "Save capture file before closing?", style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if wx.ID_YES == dialog.ShowModal():
                save_dialog = wx.FileDialog(self.frame, "Save capture file as:", wildcard="*.dat", style=wx.SAVE|wx.OVERWRITE_PROMPT)
                if save_dialog.ShowModal() == wx.ID_OK:
                    path = save_dialog.GetPath()
                    save_dialog.Destroy()
                    # ToDo move (link/unlink) the capture to path
                    # ToDo write the info file

        self._set_state("STOPPED")

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

    # Open an existing capture file
    #
    def open_file(self, capture_file):
        try:
            self.__read_file_properties(capture_file + ".info")
            capture_rate = self.info["capture-rate"]
            self.__set_rx_from_file(capture_file, capture_rate)
            self._set_titlebar(capture_file)
            self._set_state("RUNNING")
        except:
            wx.MessageBox("Cannot open capture file.", "File Error", wx.CANCEL | wx.ICON_EXCLAMATION)

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


# Start the receiver
#
if '__main__' == __name__:
    app = stdgui2.stdapp(p25_rx_block, "APCO P25 Receiver", 3)
    app.MainLoop()
