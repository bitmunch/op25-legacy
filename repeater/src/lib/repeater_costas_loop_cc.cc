/* -*- c++ -*- */
/*
 * Copyright 2006 Free Software Foundation, Inc.
 * 
 * PI/4 DQPSK hack Copyright 2010 KA1RBI
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

#include <repeater_costas_loop_cc.h>
#include <gr_io_signature.h>
#include <gr_expj.h>
#include <gr_sincos.h>
#include <math.h>

#define M_TWOPI (2*M_PI)
static const gr_complex PT_45 = gr_expj( M_PI / 4.0 );

repeater_costas_loop_cc_sptr
repeater_make_costas_loop_cc (float alpha, float beta,
			float max_freq, float min_freq,
			int order
			) throw (std::invalid_argument)
{
  return repeater_costas_loop_cc_sptr (new repeater_costas_loop_cc (alpha, beta,
							max_freq, min_freq,
							order));
}

repeater_costas_loop_cc::repeater_costas_loop_cc (float alpha, float beta,
				      float max_freq, float min_freq,
				      int order
				      ) throw (std::invalid_argument)
  : gr_sync_block ("costas_loop_cc",
		   gr_make_io_signature (1, 1, sizeof (gr_complex)),
		   gr_make_io_signature (1, 2, sizeof (gr_complex))),
    d_alpha(alpha), d_beta(beta), 
    d_max_freq(max_freq), d_min_freq(min_freq),
    d_phase(0), d_freq((max_freq+min_freq)/2),
    d_order(order), d_phase_detector(0),
    d_interp_counter(0), d_rot45(false)
{
  if (d_order < 0) {
    d_rot45 = true;
    d_order = 0 - d_order;
  }
  switch(d_order) {
  case 2:
    d_phase_detector = &repeater_costas_loop_cc::phase_detector_2;
    break;

  case 4:
    d_phase_detector = &repeater_costas_loop_cc::phase_detector_4;
    break;

  default: 
    throw std::invalid_argument("order must be 2 or 4");
    break;
  }
}


float
repeater_costas_loop_cc::phase_detector_4(gr_complex sample) const
{

  return ((sample.real()>0 ? 1.0 : -1.0) * sample.imag() -
	  (sample.imag()>0 ? 1.0 : -1.0) * sample.real());
}

float
repeater_costas_loop_cc::phase_detector_2(gr_complex sample) const
{
  return (sample.real()*sample.imag());
}

void
repeater_costas_loop_cc::set_alpha(float alpha)
{
  d_alpha = alpha;
}

void
repeater_costas_loop_cc::set_beta(float beta)
{
  d_beta = beta;
}

int
repeater_costas_loop_cc::work (int noutput_items,
			 gr_vector_const_void_star &input_items,
			 gr_vector_void_star &output_items)
{
  const gr_complex *iptr = (gr_complex *) input_items[0];
  gr_complex *optr = (gr_complex *) output_items[0];
  gr_complex *foptr = (gr_complex *) output_items[1];

  bool write_foptr = output_items.size() >= 2;

  float error;
  gr_complex nco_out;
  gr_complex rotated_sample;
  
  if (write_foptr) {

    for (int i = 0; i < noutput_items; i++){
      nco_out = gr_expj(-d_phase);
      rotated_sample = optr[i] = iptr[i] * nco_out;

      if (d_rot45) {
        if (d_interp_counter & 1)    // every other symbol
          rotated_sample = rotated_sample * PT_45;    // rotate by +45 deg
        d_interp_counter++;
      }

      error = (*this.*d_phase_detector)(rotated_sample);
      if (error > 1)
	error = 1;
      else if (error < -1)
	error = -1;
      
      d_freq = d_freq + d_beta * error;
      d_phase = d_phase + d_freq + d_alpha * error;
      
      while(d_phase>M_TWOPI)
	d_phase -= M_TWOPI;
      while(d_phase<-M_TWOPI)
	d_phase += M_TWOPI;
      
      if (d_freq > d_max_freq)
	d_freq = d_max_freq;
      else if (d_freq < d_min_freq)
	d_freq = d_min_freq;
      
      foptr[i] = gr_complex(d_freq,0);
    } 
  } else {
    for (int i = 0; i < noutput_items; i++){
      nco_out = gr_expj(-d_phase);
      rotated_sample = optr[i] = iptr[i] * nco_out;

      if (d_rot45) {
        if (d_interp_counter & 1)    // every other symbol
          rotated_sample = rotated_sample * PT_45;    // rotate by +45 deg
        d_interp_counter++;
      }

      error = (*this.*d_phase_detector)(rotated_sample);
      if (error > 1)
	error = 1;
      else if (error < -1)
	error = -1;
      
      d_freq = d_freq + d_beta * error;
      d_phase = d_phase + d_freq + d_alpha * error;
      
      while(d_phase>M_TWOPI)
	d_phase -= M_TWOPI;
      while(d_phase<-M_TWOPI)
	d_phase += M_TWOPI;
      
      if (d_freq > d_max_freq)
	d_freq = d_max_freq;
      else if (d_freq < d_min_freq)
	d_freq = d_min_freq;
      
    }
  }
  return noutput_items;
}
