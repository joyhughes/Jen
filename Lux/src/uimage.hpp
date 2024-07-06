#ifndef __UIMAGE_HPP
#define __UIMAGE_HPP

#include "image.hpp"


#define uimage image< ucolor >

// pixel modification functions using template specialization
template<> void uimage::grayscale();
template<> void uimage::rotate_colors( const int& r );
template<> void uimage::invert();
template<> void uimage::rgb_to_hsv();
template<> void uimage::hsv_to_rgb();

// I/O functions using template specialization
template<> void uimage::load( const std::string& filename );
template<> void uimage::write_jpg( const std :: string& filename, int quality );
template<> void uimage::write_png( const std :: string& filename );

template<> const ucolor image< ucolor >::sample ( const unsigned int mip_level, const unsigned int mip_blend, const vec2i& vi ) const;

template<> void uimage::dump();

#endif // __UIMAGE_HPP
