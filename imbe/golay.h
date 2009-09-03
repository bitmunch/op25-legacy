#ifndef INCLUDED_GOLAY_H
#define INCLUDED_GOLAY_H

#include <cstddef>
#include <stdint.h>

/* APCO Golay(23,11,7) ecoder.
 *
 * \param val The 12-bit value to encode.
 * \return The encoded codeword.
 */
extern uint32_t golay_23_encode(uint32_t);

/* APCO Golay(23,11,7) decoder.
 *
 * \param cw The 23-bit codeword to decode.
 * \return The number of errors detected.
 */
extern size_t golay_23_decode(uint32_t& cw);

#endif /* INCLUDED_GOLAY_H */
