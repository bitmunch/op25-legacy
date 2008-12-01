#ifndef INCLUDED_SWAB_H
#define INCLUDED_SWAB_H

#include <bitset>
#include <itpp/base/vec.h>
#include <stdexcept>

/**
 * Construct a bitset<M> from a bitset<N>.
 *
 * \param b A bitset to copy from.
 * \return A new bitset<M> initialized from b[start,M);
 */
template<size_t M, size_t N>
std::bitset<M>
extract(const std::bitset<N>& b, const size_t begin)
{
	// ToDo: argument checking
	unsigned long val = (b.to_ulong() >> (N - (begin + M))) & ((1 << M) - 1);
	return std::bitset<M>(val);
}

/**
 * Swab bits from bitset to bvec. The source bit vector in[begin,end)
 * is copied to out at the specified position and can be done in
 * reverse.
 *
 * \param in A const reference to the bvec bit source.
 * \param begin The offset of the first bit to copy.
 * \param end The offset of rhe end bit (just beyond last bit).
 * \param out The bvec into which bits are written.
 * \param where The starting point for writing bits.
 */
template<size_t N>
void
swab(const std::bitset<N>& in, size_t begin, size_t end, itpp::bvec& out, size_t where)
{
	// ToDo: argument checking
	for(size_t i = begin, j = where; i != end; (begin < end ? ++i : --i), ++j) {
		out[j] = in[i];
	}
}

/**
 * Swab bits from bitset to bvec. The source bvec in[begin,end)
 * is copied to out at the specified position and can be done in
 * reverse.
 *
 * \param in A const reference to the bvec bit source.
 * \param begin The offset of the first bit to copy.
 * \param end The offset of rhe end bit (just beyond last bit).
 * \param out The bvec into which bits are written.
 * \param where The starting point for writing bits.
 */
template<size_t N>
void
swab(const itpp::bvec& in, size_t begin, size_t end, std::bitset<N>& out, size_t where)
{
	// ToDo: argument checking
	for(size_t i = begin, j = where; i != end; (begin < end ? ++i : --i), ++j) {
		out[j] = in[i];
	}
}

#endif // INCLUDED_SWAB_H
