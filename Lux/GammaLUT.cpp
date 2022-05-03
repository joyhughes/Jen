#include "GammaLUT.hpp"
#include "srgb.hpp"
#include <array>

static std::array<float, 256> buildGammaLUTLookupTable()
{
  std::array<float, 256> result;
  for (int i = 0; i < 256; ++i)
  {
    result[i] = srgb::srgb_to_linear(i);
  }
  return result;
}

static std::array<float, 256> lut = buildGammaLUTLookupTable();

float GammaLUT::lookup(unsigned char index)
{
  return lut[index];
}

frgb GammaLUT::lookup(unsigned char r, unsigned char g, unsigned char b)
{
  return frgb(lookup(r), lookup(g), lookup(b));
}

// frgb lookup( unsigned int c ) { bit shifty stuff }

// Reverse lookup? Quick tree search