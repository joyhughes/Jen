#ifndef __GAMMA_LUT_HPP
#define __GAMMA_LUT_HPP
#include "frgb.hpp"

class gamma_lut {
    float gamma;

public:
    gamma_lut( float gamma );

    float lookup( unsigned char index );
    frgb  lookup( unsigned char r, unsigned char g, unsigned char b );
};

#endif