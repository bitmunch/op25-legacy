#ifndef INCLUDED_IMBE_HEADER_H
#define INCLUDED_IMBE_HEADER_H

#include <cstddef>
#include <stdint.h>
#include <abstract_data_unit.h>

static const size_t nof_voice_codewords = 9, voice_codeword_sz = 144;

/* APCO IMBE header decoder.
 *
 * extracts 88 bits of IMBE parameters given an input 144-bit frame
 *
 * \param cw Codeword containing bits to be decoded
 * \param u0-u7 Result output vectors
 */

extern void imbe_header_decode(const voice_codeword& cw, uint32_t& u0, uint32_t& u1, uint32_t& u2, uint32_t& u3, uint32_t& u4, uint32_t& u5, uint32_t& u6, uint32_t& u7, uint32_t& E0, uint32_t& ET);

/* APCO IMBE header encoder.
 *
 * given 88 bits of IMBE parameters, construct output 144-bit IMBE codeword
 *
 * \param cw Codeword into which 144 bits of results are placed
 * \param u0-u7 input parameters
 */

extern void imbe_header_encode(voice_codeword& cw, uint32_t u0, uint32_t u1, uint32_t u2, uint32_t u3, uint32_t u4, uint32_t u5, uint32_t u6, uint32_t u7);

extern void imbe_deinterleave (const_bit_vector& frame_body, voice_codeword& cw, uint32_t frame_nr);
extern void imbe_interleave(bit_vector& frame_body, const voice_codeword& cw, uint32_t frame_nr);

uint32_t pngen15(uint32_t& pn);
uint32_t pngen23(uint32_t& pn);

void imbe_frame_unpack(uint8_t *A, uint32_t& u0, uint32_t& u1, uint32_t& u2, uint32_t& u3, uint32_t& u4, uint32_t& u5, uint32_t& u6, uint32_t& u7, uint32_t& E0, uint32_t& ET);

#endif /* INCLUDED_IMBE_HEADER_H */
