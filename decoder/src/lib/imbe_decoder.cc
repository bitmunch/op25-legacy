// #include <dummy_imbe_decoder.h>
#include <imbe_decoder.h>
#include <offline_imbe_decoder.h>
// #include <software_imbe_decoder.h>
// #include <vc55_imbe_decoder.h>

imbe_decoder_sptr
imbe_decoder::make_imbe_decoder()
{
   return imbe_decoder_sptr(new offline_imbe_decoder());
}

imbe_decoder::~imbe_decoder()
{
}

imbe_decoder::imbe_decoder() :
   d_audio()
{
}

audio_samples*
imbe_decoder::audio()
{
   return &d_audio;
}
