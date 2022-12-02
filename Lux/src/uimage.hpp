#ifndef __UIMAGE_HPP
#define __UIMAGE_HPP

#include "ucolor.hpp"
#include "image.hpp"

class uimage : public image< ucolor > {

protected:
    void spool( std :: vector< unsigned char >& img );

public:
    uimage() : image< ucolor >() {}     
    uimage( const vec2i& dims ) : image< ucolor >( dims ){}     // creates image of particular size 
    uimage( const vec2i& dims, const bb2f& bb ) : image< ucolor >( dims, bb ) {}     
    uimage( const I& img ) : image< ucolor >( img ) {}          // copy constructor
    uimage( const std::string& filename ) : image< ucolor >() { load( filename ); } 
  
    // pixel modification functions
    void grayscale();

    // I/O functions
    void load(      const std :: string& filename );
    void write_jpg( const std :: string& filename, int quality );
    void write_png( const std :: string& filename );
};

#endif // __UIMAGE_HPP