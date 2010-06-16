/* -*- c++ -*- */

%feature("autodoc", "1");		// generate python docstrings

%include "exception.i"
%import "gnuradio.i"			// the common stuff

%{
#include "gnuradio_swig_bug_workaround.h"	// mandatory bug fix
#include "repeater_squelch_base_ff.h"
#include "repeater_fsk4_slicer_fb.h"
#include "repeater_p25_frame_fb.h"
#include "repeater_imbe_decode_fb.h"
#include "repeater_pipe.h"
#include "repeater_ctcss_squelch_ff.h"
#include "repeater_gardner_symbol_recovery_cc.h"
#include "repeater_costas_loop_cc.h"
#include <stdexcept>
%}

// ----------------------------------------------------------------

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 *
 * This does some behind-the-scenes magic so we can
 * access repeater_fsk4_slicer_fb from python as repeater.fsk4_slicer_fb
 */

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,fsk4_slicer_fb);

repeater_fsk4_slicer_fb_sptr repeater_make_fsk4_slicer_fb ();

class repeater_fsk4_slicer_fb : public gr_sync_block
{
private:
  repeater_fsk4_slicer_fb ();
};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,p25_frame_fb);

repeater_p25_frame_fb_sptr repeater_make_p25_frame_fb ();

class repeater_p25_frame_fb : public gr_sync_block
{
private:
  repeater_p25_frame_fb ();
};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,imbe_decode_fb);

repeater_imbe_decode_fb_sptr repeater_make_imbe_decode_fb ();

class repeater_imbe_decode_fb : public gr_sync_block
{
private:
  repeater_imbe_decode_fb ();
};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,pipe);

repeater_pipe_sptr repeater_make_pipe (size_t input_size, size_t output_size, const char* pathname, float io_ratio);

class repeater_pipe : public gr_sync_block
{
private:
  repeater_pipe (size_t input_size, size_t output_size, const char* pathname, float io_ratio);
};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,squelch_base_ff);

class repeater_squelch_base_ff : public gr_block
{
private:
  enum { ST_MUTED, ST_ATTACK, ST_UNMUTED, ST_DECAY } d_state;

public:
  repeater_squelch_base_ff(const char *name, int ramp, bool gate);

  int ramp() const { return d_ramp; }
  void set_ramp(int ramp) { d_ramp = ramp; }
  bool gate() const { return d_gate; }
  void set_gate(bool gate) { d_gate = gate; }
  bool unmuted() const { return (d_state == ST_UNMUTED || d_state == ST_ATTACK); }

  virtual std::vector<float> squelch_range() const = 0;
};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,ctcss_squelch_ff);

repeater_ctcss_squelch_ff_sptr repeater_make_ctcss_squelch_ff (int rate, float freq, float level=5.0, int len=0, int ramp=0, bool gate=false);

class repeater_ctcss_squelch_ff : public repeater_squelch_base_ff
{
private:
  repeater_ctcss_squelch_ff ();

public:
  std::vector<float> squelch_range() const;
  float level() const { return d_level; }
  void set_level(float level) { d_level = level; }
  int len() const { return d_len; }

};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,gardner_symbol_recovery_cc);

repeater_gardner_symbol_recovery_cc_sptr repeater_make_gardner_symbol_recovery_cc (int samples_per_symbol, float timing_error_gain);

class repeater_gardner_symbol_recovery_cc : public gr_sync_block
{
 private:
  repeater_gardner_symbol_recovery_cc (int samples_per_symbol, float timing_error_gain);
};

// ----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC(repeater,costas_loop_cc);

repeater_costas_loop_cc_sptr
repeater_make_costas_loop_cc (float alpha, float beta, 
			float max_freq, float min_freq,
			int order
			) throw (std::invalid_argument);


class repeater_costas_loop_cc : public gr_sync_block
{
 private:
  repeater_costas_loop_cc (float alpha, float beta,
		     float max_freq, float min_freq, int order);

 public:
   void set_alpha(float alpha);
   float alpha();
   void set_beta(float beta);
   float beta();
   float freq();   
};
