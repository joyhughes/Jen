#ifndef __GAMMA_LUT_HPP
#define __GAMMA_LUT_HPP
#include "frgb.hpp"

class GammaLUT {
    float gamma;

    // constructor
    GammaLUT( float gamma ) {
        this->gamma = gamma;
    }

    float lookup( unsigned char index );
    frgb  lookup( unsigned char r, unsigned char g, unsigned char b );
    //frgb lookup( unsigned int c ) { bit shifty stuff }

    // Reverse lookup? Quick tree search
};

#endif