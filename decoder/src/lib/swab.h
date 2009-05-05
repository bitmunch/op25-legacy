#ifndef INCLUDED_SWAB_H
#define INCLUDED_SWAB_H

#include <bitset>
#include <itpp/base/vec.h>
#include <vector>

typedef std::vector<bool> bit_vector;
typedef const std::vector<bool> const_bit_vector;

/**
 * Swab in[bits[0)..bits[bits_sz)) to out[where..where+bits_sz).
 *
 * \param in A const reference to the source.
 * \param bits An array specifying the ordinals of the bits to copy.
 * \param bits_sz The size of the bits array.
 * \param out A reference to the destination.
 * \param where The offset of the first bit to write.
 */
template <class X, class Y>
void swab(const X& in, const size_t bits[], size_t bits_sz, Y& out, size_t where)
{
   for(size_t i = 0; i < bits_sz; ++i) {
      out[where+i] = in[bits[i]];
   }
}

/**
 * Swab from in[0..bits_sz) to out[bits[0)..bits[bits_sz)).
 *
 * \param in A const reference to the source.
 * \param where The offset of the first bit to read.
 * \param out A reference to the destination.
 * \param bits An array specifying the ordinals of the bits to copy.
 * \param bits_sz The size of the bits array.
 */
template <class X, class Y>
void unswab(const X& in, size_t where, Y& out, const size_t bits[], size_t bits_sz)
{
   for(size_t i = 0; i < bits_sz; ++i) {
      out[bits[i]] = in[where+i];
   }
}

/**
 * Extract a bit_vector in[begin,end) to an octet buffer.
 *
 * \param in A const reference to the source.
 * \param begin The offset of the first bit to extract (the MSB).
 * \param end The offset of the end bit.
 * \param out Address of the octet buffer to write into.
 * \return The number of octers written.
 */
template<class X>
size_t extract(const X& in, int begin, int end, uint8_t *out)
{
   const size_t out_sz = (7 + end - begin) / 8;
   memset(out, 0, out_sz);
   for(int i = begin, j = 0; i < end; ++i, ++j) {
      out[j / 8] ^= in[i] << (7 - (j % 8));
   }
   return out_sz;
}

/**
 * Extract a bit_vector in[bits[0)..bits[bits_sz)) to an octet buffer.
 *
 * \param in A const reference to the source.
 * \param bits An array specifying the ordinals of the bits to extract.
 * \param bits_sz The size of the bits array.
 * \param out Address of the octet buffer to write into.
 * \return The number of octers written.
 */
template<class X>
size_t extract(const X& in, const size_t bits[], size_t bits_sz, uint8_t *out)
{
   const size_t out_sz = (7 + bits_sz) / 8;
   memset(out, 0, out_sz);
   for(size_t i = 0; i < bits_sz; ++i) {
      out[i / 8] ^= in[bits[i]] << (7 - (i % 8));
   }
   return out_sz;
}

/**
 * Extract value of bits from in[bits[0..bits_sz)).
 *
 * \param in The input const_bit_vector.
 * \param begin The offset of the first bit to extract (the MSB).
 * \param end The offset of the end bit.
 * \return A uint64_t containing the value
 */
template<class X>
uint64_t extract(const X& in, int begin, int end)
{
   uint64_t x = 0LL;
   for(int i = begin; i < end; ++i) {
      x =  (x << 1) | (in[i] ? 1 : 0);
   }
   return x;
}

/**
 * Extract value of bits from in[bits[0..bits_sz)).
 *
 * \param in A const reference to the source.
 * \param bits An array specifying the ordinals of the bits to extract.
 * \param bits_sz The size of the bits array.
 * \return A uint64_t containing the value
 */
template<class X>
uint64_t extract(const X& in, const size_t bits[], size_t bits_sz)
{
   uint64_t x = 0LL;
   for(size_t i = 0; i < bits_sz; ++i) {
      x =  (x << 1) | (in[bits[i]] ? 1 : 0);
   }
   return x;
}

#endif // INCLUDED_SWAB_H
