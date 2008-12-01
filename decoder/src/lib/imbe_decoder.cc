#include <dummy_imbe_decoder.h>
#include <imbe_decoder.h>

imbe_decoder_sptr
imbe_decoder::make_imbe_decoder()
{
   return imbe_decoder_sptr(new dummy_imbe_decoder());
}

imbe_decoder::~imbe_decoder()
{
}

imbe_decoder::imbe_decoder()
{
}