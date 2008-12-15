#ifndef INCLUDED_SWAB_H
#define INCLUDED_SWAB_H

#include <bitset>
#include <itpp/base/vec.h>
#include <stdexcept>
#include <vector>

typedef std::vector<bool> bit_vector;
typedef const std::vector<bool> const_bit_vector;

/**
 * Swab bits from bitset in[begin,end) to bit_vector at position where.
 *
 * \param in A const reference to the bitset.
 * \param begin The offset of the first bit to copy.
 * \param end The offset of the end bit.
 * \param out The bit_vector into which bits are written.
 * \param where The starting point for writing bits.
 */
template<size_t N> void
swab(const std::bitset<N>& in, int begin, int end, bit_vector& out, int where)
{
	for(int i = begin, j = where; i != end; (begin < end ? ++i : --i), ++j) {
		out[j] = in[i];
	}
}

/**
 * Swab bits from bitset in[begin,end) to octet vector out at position
 * where. Packing of out starts with the MSB.
 *
 * \param in A const reference to the bitset.
 * \param begin The offset of the first bit to copy.
 * \param end The offset of the end bit.
 * \param out Pointer to octet array into which bits are written.
 * \param where The starting point for writing bits.
 */
template<size_t N> void
swab(const std::bitset<N>& in, int begin, int end, uint8_t *out, int where)
{
	for(int i = begin, j = 0; i != end; (begin < end ? ++i : --i), ++j) {
		out[where + (j / 8)] ^= in[i] << (7 - (j % 8));
	}
}

/**
 * Swab bits from bit_vector in[begin,end) to bvec at position where.
 *
 * \param in A const reference to the bitset.
 * \param begin The offset of the first bit to copy.
 * \param end The offset of the end bit.
 * \param out A bvec into which bits are written.
 * \param where The starting point for writing bits.
 */
inline void
swab(const_bit_vector& in, int begin, int end, itpp::bvec& out, int where)
{
	for(int i = begin, j = where; i != end; (begin < end ? ++i : --i), ++j) {
		out[j] = in[i];
	}
}

/**
 * Swab bits from bitset in[begin,end) to octet vector out at position
 * where. Packing of out starts with the MSB.
 *
 * \param in A const reference to the bitset.
 * \param begin The offset of the first bit to copy.
 * \param end The offset of the end bit.
 * \param out Pointer to octet array into which bits are written.
 * \param where The starting point for writing bits.
 */
inline void
swab(const_bit_vector& in, int begin, int end, uint8_t *out, int where)
{
	for(int i = begin, j = 0; i != end; (begin < end ? ++i : --i), ++j) {
		out[where + (j / 8)] ^= in[i] << (7 - (j % 8));
	}
}

/**
 * Swab bits from bit_vector in[begin,end) to bvec at position where.
 *
 * \param in A const reference to the bitset.
 * \param begin The offset of the first bit to copy.
 * \param end The offset of the end bit.
 * \param out A bvec into which bits are written.
 * \param where The starting point for writing bits.
 */
inline void
swab(const itpp::bvec& in, int where, bit_vector& out, int begin, int end)
{
	for(int i = begin, j = where; i != end; (begin < end ? ++i : --i), ++j) {
		out[i] = in[j];
	}
}

/**
 * Extract value of bits from in[begin,end).
 *
 * \param in The input const_bit_vector.
 * \param begin The offset of the first bit to extract (the MSB).
 * \param end The offset of the end bit.
 * \return 
 */
inline uint64_t
extract(const_bit_vector& in, int begin, int end)
{
	uint64_t x = 0LL;
	for(int i = begin; i != end; (begin < end ? ++i : --i)) {
		x =  (x << 1) | (in[i] ? 1 : 0);
	}
	return x;
}


#endif // INCLUDED_SWAB_H
