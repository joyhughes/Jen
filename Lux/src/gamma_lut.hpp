// Look up table to convert between floating-point and integer RGB colors

#ifndef __GAMMA_LUT_HPP
#define __GAMMA_LUT_HPP

#include <array>

// Number of bits in linear to SRGB lookup table
constexpr unsigned int lts_bits = 12;
// Number of bits to shift between index and floating-point mask
constexpr unsigned int lts_shift = 27 - lts_bits;
// Number of useful entries in linear to SRGB lookup table - 3841 for 12 bits
constexpr unsigned int lts_entries = ( 1 << lts_bits ) - ( 1 << ( lts_bits - 4 ) ) + 1;

class gamma_LUT {
    float gamma;
    std::array< float, 256 > SRGB_to_linear_LUT;
    std::array< unsigned char, lts_entries >  linear_to_SRGB_LUT;

public:
    gamma_LUT( float gamma );

    float SRGB_to_linear( unsigned char index );
    unsigned char linear_to_SRGB( float index );
};

#endif // __GAMMA_LUT_HPP
