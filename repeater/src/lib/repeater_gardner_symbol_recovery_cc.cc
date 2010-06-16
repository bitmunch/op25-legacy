/* -*- c++ -*- */
/*
 * Copyright 2005,2006 Free Software Foundation, Inc.
 *
 * Gardner symbol recovery block for GR - Copyright 2010, KA1RBI
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_io_signature.h>
#include <gr_prefs.h>
#include <gr_math.h>
#include <gr_expj.h>
#include <repeater_gardner_symbol_recovery_cc.h>
#include <gri_mmse_fir_interpolator_cc.h>
#include <stdexcept>
#include <cstdio>
#include <string.h>

#define M_TWOPI (2*M_PI)

// Public constructor

repeater_gardner_symbol_recovery_cc_sptr 
repeater_make_gardner_symbol_recovery_cc(int samples_per_symbol, float timing_error_gain)
{
  return repeater_gardner_symbol_recovery_cc_sptr (new repeater_gardner_symbol_recovery_cc (samples_per_symbol, timing_error_gain));
}

repeater_gardner_symbol_recovery_cc::repeater_gardner_symbol_recovery_cc (int samples_per_symbol, float timing_error_gain)
  : gr_block ("repeater_gardner_symbol_recovery_cc",
	      gr_make_io_signature (1, 1, sizeof (gr_complex)),
	      gr_make_io_signature (1, 2, sizeof (gr_complex))),
	      d_last_sample(0), d_interp(new gri_mmse_fir_interpolator_cc()),
    d_verbose(gr_prefs::singleton()->get_bool("repeater_gardner_symbol_recovery_cc", "verbose", false)),
    d_dl_index(0),
    d_samples_per_symbol(samples_per_symbol), d_timing_error_gain(timing_error_gain), 
    d_timing_error(0)
{
  assert (d_samples_per_symbol >= 2);
  assert ((d_samples_per_symbol & 1) == 0);   // sps must be even

  d_half_sps = d_samples_per_symbol >> 1;
  d_twice_sps = d_samples_per_symbol * 2;
  set_relative_rate (1.0 / d_samples_per_symbol);
  set_history(d_twice_sps);			// ensure extra input is available
  d_dl = new gr_complex[d_twice_sps*2];
}

repeater_gardner_symbol_recovery_cc::~repeater_gardner_symbol_recovery_cc ()
{
  delete d_interp;
  delete d_dl;
}

void
repeater_gardner_symbol_recovery_cc::forecast(int noutput_items, gr_vector_int &ninput_items_required)
{
  unsigned ninputs = ninput_items_required.size();
  for (unsigned i=0; i < ninputs; i++)
    ninput_items_required[i] =
      (int) ceil((noutput_items * d_samples_per_symbol) + d_interp->ntaps());
}

// sample processing - includes slip by +/- 1 sample if error is excessive
bool
repeater_gardner_symbol_recovery_cc::input_sample0(gr_complex samp, gr_complex& outp) {
	bool rc0=false, rc1=false;

	if (d_timing_error >= 0.5) {
		d_timing_error = -0.5;
		return false;   // skip this samp
	}
	if (d_timing_error <= -0.5) {
		d_timing_error =  0.5;
		rc0 = input_sample(samp, outp);   // repeat this samp
	}

	rc1 = input_sample(samp, outp);
	return rc0 | rc1;
}

// returns true if an output sample was generated and stored in outp
bool
repeater_gardner_symbol_recovery_cc::input_sample(gr_complex samp, gr_complex& outp) {
	bool rc = false;

	d_dl[d_dl_index] = samp;
	d_dl[d_dl_index + d_twice_sps] = samp;
	d_dl_index ++;
	d_dl_index = d_dl_index % d_twice_sps;

	if ((d_dl_index % d_samples_per_symbol) == 0) {
		float mu = 0.5 + d_timing_error;
		if (mu < 0.0) mu = 0.0;
		if (mu > 1.0) mu = 1.0;
		gr_complex interp_samp_mid = d_interp->interpolate(&d_dl[ d_dl_index ], mu);
		gr_complex interp_samp = d_interp->interpolate(&d_dl[ d_dl_index + d_half_sps], mu);
		outp = interp_samp;
		float error_real = (d_last_sample.real() - interp_samp.real()) * interp_samp_mid.real();
		float error_imag = (d_last_sample.imag() - interp_samp.imag()) * interp_samp_mid.imag();
		d_last_sample = interp_samp;
		float symbol_error = error_real + error_imag;
		if (symbol_error < -1.0) symbol_error = -1.0;
		if (symbol_error >  1.0) symbol_error =  1.0;
		d_timing_error = d_timing_error + d_timing_error_gain * symbol_error;
		// printf("%f\t%f\t%f\n", symbol_error, d_timing_error, mu);
		rc = true;
	}
	return rc;
}

int
repeater_gardner_symbol_recovery_cc::general_work (int noutput_items,
				   gr_vector_int &ninput_items,
				   gr_vector_const_void_star &input_items,
				   gr_vector_void_star &output_items)
{
  const gr_complex *in = (const gr_complex *) input_items[0];
  gr_complex *out = (gr_complex *) output_items[0];

  int i=0, o=0;

  while((o < noutput_items) && (i < ninput_items[0])) {
     gr_complex interp_sample;
     if(input_sample0(in[i++], interp_sample)) {
      out[o++] = interp_sample;
     }
  }

  consume_each(i);
  return o;
}
