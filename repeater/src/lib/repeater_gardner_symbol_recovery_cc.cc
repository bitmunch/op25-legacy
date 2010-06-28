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

// Public constructor

repeater_gardner_symbol_recovery_cc_sptr 
repeater_make_gardner_symbol_recovery_cc(float samples_per_symbol, float timing_error_gain)
{
  return repeater_gardner_symbol_recovery_cc_sptr (new repeater_gardner_symbol_recovery_cc (samples_per_symbol, timing_error_gain));
}

repeater_gardner_symbol_recovery_cc::repeater_gardner_symbol_recovery_cc (float samples_per_symbol, float timing_error_gain)
  : gr_block ("repeater_gardner_symbol_recovery_cc",
	      gr_make_io_signature (1, 1, sizeof (gr_complex)),
	      gr_make_io_signature (1, 1, sizeof (gr_complex))),
    d_mu(0),
    d_gain_mu(timing_error_gain),
    d_last_sample(0), d_interp(new gri_mmse_fir_interpolator_cc()),
    d_verbose(gr_prefs::singleton()->get_bool("repeater_gardner_symbol_recovery_cc", "verbose", false)),
    d_dl_index(0)
{
  set_omega(samples_per_symbol);
  d_gain_omega = 0.25 * d_gain_mu * d_gain_mu;	// FIXME: parameterize this
  set_relative_rate (1.0 / d_omega);
  set_history(d_twice_sps);			// ensure extra input is available
  d_dl = new gr_complex[d_twice_sps*2];
}

void repeater_gardner_symbol_recovery_cc::set_omega (float omega) {
    assert (omega >= 2.0);
    d_omega = omega;
    d_min_omega = omega*(1.0 - d_omega_rel);
    d_max_omega = omega*(1.0 + d_omega_rel);
    d_omega_mid = 0.5*(d_min_omega+d_max_omega);
    d_twice_sps = 2 * (int) ceilf(d_omega);
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
      (int) ceil((noutput_items * d_omega) + d_interp->ntaps());
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
    while((d_mu > 1.0) && (i < ninput_items[0]))  {
	d_mu --;

	d_dl[d_dl_index] = in[i];
	d_dl[d_dl_index + d_twice_sps] = in[i];
	d_dl_index ++;
	d_dl_index = d_dl_index % d_twice_sps;

	i++;
    }
    
    if(d_mu <= 1.0) {
		float half_omega = d_omega / 2.0;
		int half_sps = (int) floorf(half_omega);
		float half_mu = d_mu + half_omega - (float) half_sps;
		if (half_mu > 1.0) {
			half_mu -= 1.0;
			half_sps += 1;
		}
		// locate two points, separated by half of one symbol time
		// interp_samp is (we hope) at the optimum sampling point 
		gr_complex interp_samp_mid = d_interp->interpolate(&d_dl[ d_dl_index ], d_mu);
		gr_complex interp_samp = d_interp->interpolate(&d_dl[ d_dl_index + half_sps], half_mu);

		float error_real = (d_last_sample.real() - interp_samp.real()) * interp_samp_mid.real();
		float error_imag = (d_last_sample.imag() - interp_samp.imag()) * interp_samp_mid.imag();
		d_last_sample = interp_samp;	// save for next time
		float symbol_error = error_real + error_imag; // Gardner loop error
		if (symbol_error < -1.0) symbol_error = -1.0;
		if (symbol_error >  1.0) symbol_error =  1.0;

		d_omega = d_omega + d_gain_omega * symbol_error;  // update omega based on loop error
		d_omega = d_omega_mid + gr_branchless_clip(d_omega-d_omega_mid, d_omega_rel);   // make sure we don't walk away

		// printf("%f\t%f\t%f\n", symbol_error, d_mu, d_omega);
		d_mu += d_omega + d_gain_mu * symbol_error;   // update mu based on loop error
  
      out[o++] = interp_samp;
    }
  }

  #if 0
  printf("ninput_items: %d   noutput_items: %d   consuming: %d   returning: %d\n",
	 ninput_items[0], noutput_items, i, o);
  #endif

  consume_each(i);
  return o;
}
