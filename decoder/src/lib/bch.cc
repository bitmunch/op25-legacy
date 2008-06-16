/* -*- C++ -*- */
/*
 * Copyright 2008 Steve Glass
 * 
 * This file is part of OP25.
 * 
 * OP25 is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * OP25 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with OP25; see the file COPYING. If not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Boston, MA
 * 02110-1301, USA.
 */

#include <bch.h>

/*
 * APCO P25 BCH(64,16,23) encoder.
 */
uint64_t
bch_64_encode(uint16_t val)
{
   static const uint64_t encoding_matrix[] = {
      0x8000cd930bdd3b2aLL,
      0x4000ab5a8e33a6beLL,
      0x2000983e4cc4e874LL,
      0x10004c1f2662743aLL,
      0x0800eb9c98ec0136LL,
      0x0400b85d47ab3bb0LL,
      0x02005c2ea3d59dd8LL,
      0x01002e1751eaceecLL,
      0x0080170ba8f56776LL,
      0x0040c616dfa78890LL,
      0x0020630b6fd3c448LL,
      0x00103185b7e9e224LL,
      0x000818c2dbf4f112LL,
      0x0004c1f2662743a2LL,
      0x0002ad6a38ce9afbLL,
      0x00019b2617ba7657LL
   };

   uint64_t cw = 0LL;
   for(uint8_t i=0; i < 16; ++i) {
      if(val & (1 << (15 - i))) {
         cw ^= encoding_matrix[i];
      }
   }
   return cw;
}

/*
 * APCO P25 BCH(64,16,23) decoder.
 */
bool
bch_64_decode(uint64_t& cw)
{
   int elp[24][22], s[23];
   int d[23], L[24], uLu[24];
   int locn[11], reg[12];
   int i, j, u, q, n;
   bool syndrome_error = false;
   bool decoded = true;

   static const int BCH_GF_EXP[64] = {
       1,  2,  4,  8, 16, 32,  3,  6, 12, 24, 48, 35,  5, 10, 20, 40, 
      19, 38, 15, 30, 60, 59, 53, 41, 17, 34,  7, 14, 28, 56, 51, 37, 
       9, 18, 36, 11, 22, 44, 27, 54, 47, 29, 58, 55, 45, 25, 50, 39, 
      13, 26, 52, 43, 21, 42, 23, 46, 31, 62, 63, 61, 57, 49, 33,  0, 
   };

   static const int BCH_GF_LOG[64] = {
      -1,  0,  1,  6,  2, 12,  7, 26,  3, 32, 13, 35,  8, 48, 27, 18, 
       4, 24, 33, 16, 14, 52, 36, 54,  9, 45, 49, 38, 28, 41, 19, 56, 
       5, 62, 25, 11, 34, 31, 17, 47, 15, 23, 53, 51, 37, 44, 55, 40, 
      10, 61, 46, 30, 50, 22, 39, 43, 29, 60, 42, 21, 20, 59, 57, 58, 
   };

   // first form the syndromes
   for(i = 1; i <= 22; i++) {
      s[i] = 0;
      for(j = 0; j <= 62; j++) {
         if(cw & (1LL << j)) {
            s[i] ^= BCH_GF_EXP[(i * j) % 63];
         }
      }
      if(s[i]) {
         syndrome_error = true;
      }
      s[i] = BCH_GF_LOG[s[i]];
   }

   // if there are errors, compute elp and try to correct them
   if(syndrome_error) {
      L[0] = 0; uLu[0] = -1; d[0] = 0;    elp[0][0] = 0;
      L[1] = 0; uLu[1] = 0;  d[1] = s[1]; elp[1][0] = 1;
      for(i = 1; i <= 21; i++) {
         elp[0][i] = -1;
         elp[1][i] = 0;
      }
      u = 0;
      do {
         ++u;
         if(-1 == d[u]) {
            L[u + 1] = L[u];
            for(i = 0; i <= L[u]; i++) {
               elp[u + 1][i] = elp[u][i];
               elp[u][i] = BCH_GF_LOG[elp[u][i]];
            }
         } else {
            // search for words with greatest uLu(q) for which d(q) != 0
            q = u - 1;
            while((-1 == d[q]) && (q > 0)) {
               --q;
            } 

            // have found first non-zero d(q)
            if(q > 0) {
               j = q;
               do {
                  --j;
                  if((d[j] != -1) && (uLu[q] < uLu[j]) ) {
                     q = j;
                  }
               } while(j > 0);
            }

            // store degree of new elp polynomial
            if(L[u] > L[q] + u - q) {
               L[u + 1] = L[u];
            } else {
               L[u + 1] = L[q] + u - q;
            }

            // form new elp(x)
            for(i = 0; i <= 21; i++) {
               elp[u + 1][i] = 0;
            }
            for(i = 0; i <= L[q]; i++) {
               if(elp[q][i] != -1) {
                  elp[u + 1][i + u - q] = BCH_GF_EXP[(d[u] + 63 - d[q] + elp[q][i]) % 63];
               }
            }
            for(i = 0; i <= L[u]; i++) {
               elp[u + 1][i] ^= elp[u][i];
               elp[u][i] = BCH_GF_LOG[elp[u][i]];
            }
         }
         uLu[u + 1] = u - L[u + 1];

         // form (u+1)th discrepancy
         if(u < 22) {
            // no discrepancy computed on last iteration
            if(s[u + 1] != -1) {
               d[u + 1] = BCH_GF_EXP[s[u + 1]];
            } else {
               d[u + 1] = 0;
            }
            for(i = 1; i <= L[u + 1]; i++) {
               if((s[u + 1 - i] != -1) && (elp[u + 1][i] != 0)) {
                  d[u + 1] ^= BCH_GF_EXP[(s[u + 1 - i] + BCH_GF_LOG[elp[u + 1][i]]) % 63];
               } 
            }
            // put d(u+1) into index form
            d[u + 1] = BCH_GF_LOG[d[u + 1]];
         }
      } while((u < 22) && (L[u + 1] <= 11));

      ++u;
      if(L[u] <= 11) { // Can correct errors
         // put elp into index form
         for(i = 0; i <= L[u]; i++) {
            elp[u][i] = BCH_GF_LOG[elp[u][i]];
         }
         // Chien search: find roots of the error locator polynomial
         for(i = 1; i <= L[u]; i++) {
            reg[i] = elp[u][i];
         }
         n = 0;
         for(i = 1; i <= 63; i++) {
            q = 1;
            for(j = 1; j <= L[u]; j++) {
               if(reg[j] != -1) {
                  reg[j] = (reg[j] + j) % 63;
                  q ^= BCH_GF_EXP[reg[j]];
               }
            }
            if(0 == q) {
               locn[n++] = 63 - i; // store root and error location number indices
            }
         }
         if(n == L[u]) {
            // no. roots = degree of elp hence <= t errors
            for(i = 0; i < L[u]; i++) {
               cw ^= 1LL << (locn[i]);
            }
         } else {
            // elp has degree > t hence cannot solve
            decoded = false;
         }
      } else {
         decoded = false;
      }
   }
   return decoded;
}
