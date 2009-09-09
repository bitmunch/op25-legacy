#include <software_imbe_decoder.h>

#include <boost/scoped_ptr.hpp>
#include <cstdio>
#include <cstdlib>

using namespace boost;

/**
 * Insert bits from an octet buffer into a voice_codeword.
 *
 * \param in The octet buffer to read from.
 * \pram begin The first bit position.
 * \param end The last bit position.
 * \param out A reference to the voice_codeword to write.
 * \return The number of octers written.
 */
size_t
extract(const uint8_t *in, size_t begin, size_t end, voice_codeword& out)
{
   size_t nof_bits = 0;
   if(begin < end) {
      nof_bits = end - begin;
      out.resize(nof_bits);
      for(size_t i = begin; i < end; ++i) {
         out[i - begin] = in[i / 8] & (1 << (7 - (i % 8)));
      }
   }
   return nof_bits;
}

/**
 * Convert a file created by offline_imbe_decoder into an audio file.
 */
int
main(int ac, char **av)
{
   FILE *out = fopen("audio.pcm", "w");
	scoped_ptr<software_imbe_decoder> imbe(new software_imbe_decoder());
   while(--ac) {
      FILE *in = fopen(*++av, "r");
      if(in) {
         uint8_t buf[18];
         while(1 == fread(buf, sizeof(buf), 1, in)) {
            voice_codeword cw(144);
            extract(buf, 0, 144, cw);
            imbe->decode(cw);
            audio_samples *audio = imbe->audio();
            for(audio_samples::iterator i(audio->begin()); i != audio->end(); ++i) {
               float f = *i;
               fwrite(&f, sizeof(f), 1, out);
            }
            audio->clear();
         }
         fclose(in);
      } else {
         perror(*av);
         exit(1);
      }
   }
   fclose(out);
   return(0);
}
