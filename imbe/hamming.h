#ifndef INCLUDED_HAMMING_H
#define INCLUDED_HAMMING_H

#include <cstddef>
#include <stdint.h>

/*
 * APCO Hamming(15,11,3) ecoder.
 *
 * \param val The 11-bit value to encode.
 * \return The encoded codeword.
 */
extern uint16_t hamming_15_encode(uint16_t val);

/*
 * APCO Hamming(15,11,3) decoder.
 *
 * \param cw The 15-bit codeword to decode.
 * \return The number of errors detected.
 */
extern size_t hamming_15_decode(uint16_t& cw);

#endif /* INCLUDED_HAMMING_H */
