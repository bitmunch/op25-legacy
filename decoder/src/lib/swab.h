#ifndef INCLUDED_SWAB_H
#define INCLUDED_SWAB_H

#include <bitset>
#include <itpp/base/vec.h>
#include <stdexcept>
#include <vector>

typedef std::vector<bool> bit_vector;
typedef const std::vector<bool> const_bit_vector;

/**
 * Swab bits from in[begin, end) to out[where,...).
 *
 * \param in A uint64_t representing a set of bits.
 * \param begin The offset of the first bit to copy.
 * \param end The offset of the end bit (this bit is not copied).
 * \param out The bit_vector into which bits are copied.
 * \param where The starting point for writing bits.
 */
inline void
swab(uint64_t in, int begin, int end, bit_vector& out, int where)
{
   for(int i = begin; i != end; (begin < end ? ++i : --i)) {
      out[where++] = (in & (1 << i) ? 1 : 0);
   }
}

/**
 * Swab bits from in[begin, end) to out[where,...).
 *
 * \param in A const_bit_vector reference to the source.
 * \param begin The offset of the first bit to copy.
 * \param end The offset of the end bit (this bit is not copied).
 * \param out A bvec into which bits are copied.
 * \param where The starting point for writing bits.
 */
inline void
swab(const_bit_vector& in, int begin, int end, itpp::bvec& out, int where)
{
   for(int i = begin; i != end; (begin < end ? ++i : --i)) {
      out[where++] = in[i];
   }
}

/**
 * Swabs a bit_vector in[begin,end) to an octet buffer.
 *
 * \param in A const reference to the bit_vector.
 * \param begin The offset of the first bit to copy.
 * \param end The offset of the end bit (this bit is not copied).
 * \param out Address of the octet buffer to write into.
 * \return The number of octers written.
 */
inline size_t
swab(const_bit_vector& in, int begin, int end, uint8_t *out)
{
   const size_t out_sz = (7 + abs(end - begin)) / 8;
   memset(out, 0, out_sz);
   for(int i = begin, j = 0; i != end; (begin < end ? ++i : --i), ++j) {
      out[j / 8] ^= in[i] << (7 - (j % 8));
   }
   return out_sz;
}
/**
 * Swabs a bitset<N> in[begin,end) to an octet buffer.
 *
 * \param in A const reference to the bitset.
 * \param begin The offset of the first bit to copy.
 * \param end The offset of the end bit (this bit is not copied).
 * \param out Address of the octet buffer to write into.
 * \return The number of octers written.
 */
template <size_t N> size_t
swab(const std::bitset<N>& in, int begin, int end, uint8_t *out)
{
   const size_t out_sz = (7 + abs(end - begin)) / 8;
   memset(out, 0, out_sz);
   for(int i = begin, j = 0; i != end; (begin < end ? ++i : --i), ++j) {
      out[j / 8] ^= in[i] << (7 - (j % 8));
   }
   return out_sz;
}

/**
 * Swab bits from bvec in[where] in to out[begin,end).
 *
 * \param in A const reference to the bvec.
 * \param begin The offset of the first bit to copy.
 * \param end The offset of the end bit.
 * \param out A bvec into which bits are written.
 * \param where The starting point for writing bits.
 */
inline void
unswab(const itpp::bvec& in, int where, bit_vector& out, int begin, int end)
{
   for(int i = begin; i != end; (begin < end ? ++i : --i)) {
      out[i] = in[where++];
   }
}

/**
 * Extract value of bits from in[begin,end).
 *
 * \param in The input const_bit_vector.
 * \param begin The offset of the first bit to extract (the MSB).
 * \param end The offset of the end bit.
 * \return A uint64_t containing the value
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
