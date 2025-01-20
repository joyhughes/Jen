// Floating point image class using image template as a base

#ifndef __FIMAGE_HPP
#define __FIMAGE_HPP

#include "image.hpp"

#define fimage image< frgb >

// pixel modification functions using template specialization
template<> void fimage::grayscale();
template<> void fimage::clamp( float minc, float maxc );
template<> void fimage::constrain();
template<> void fimage::rotate_components( const int& r );
template<> void fimage::invert();
template<> void fimage::rgb_to_hsv();
template<> void fimage::hsv_to_rgb();
template<> void fimage::rotate_hue( const float& h );

// I/O functions using template specialization
template<> void fimage::load( const std::string& filename );
template<> void fimage::write_jpg( const std :: string& filename, int quality, int level );
template<> void fimage::write_png( const std :: string& filename, int level );

#endif // __FIMAGE_HPP
