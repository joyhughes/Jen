#ifndef __UIMAGE_HPP
#define __UIMAGE_HPP

#include "ucolor.hpp"
#include "image.hpp"

class uimage : public image< ucolor > {
public:
    uimage() : image< ucolor >() {}     
    uimage( const vec2i& dims ) : image< ucolor >( dims ){}     // creates image of particular size 
    uimage( const vec2i& dims, const bb2f& bb ) : image< ucolor >( dims, bb ) {}     
    uimage( const I& img ) : image< ucolor >( img ) {}          // copy constructor
    uimage( const std::string& filename );
  
    // pixel modification functions
    void grayscale();

};

// I/O functions using template specialization
template<> void image< ucolor >::load( const std::string& filename );
template<> void image< ucolor >::write_jpg( const std :: string& filename, int quality );
template<> void image< ucolor >::write_png( const std :: string& filename );
template<> void image< ucolor >::write_file( const std::string& filename, file_type type, int quality );

#endif // __UIMAGE_HPP
