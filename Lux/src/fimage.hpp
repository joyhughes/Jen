// Floating point image class using image template as a base

#ifndef __FIMAGE_HPP
#define __FIMAGE_HPP

#include "image.hpp"

#define fimage image< frgb >

// pixel modification functions using template specialization
template<> void fimage::grayscale();
template<> void fimage::clamp( float minc, float maxc );
template<> void fimage::constrain();

#endif // __FIMAGE_HPP
