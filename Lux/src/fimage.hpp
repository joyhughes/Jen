// Floating point image class using image template as a base

#ifndef __FIMAGE_HPP
#define __FIMAGE_HPP

#include "image.hpp"

class fimage : public image< frgb > {

protected:
    void quantize( std :: vector< unsigned char >& img );

public:
    fimage() : image() {}     
    // creates image of particular size 
    fimage( const vec2i& dims ) : image( dims ){}     
    fimage( const vec2i& dims, const bb2f& bb ) : image( dims, bb ) {}     
    // copy constructor
    fimage( const I& img ) : image( img ) {}     
    // pixel modification functions
    void grayscale();

    // I/O functions
    void load(      const std :: string& filename );
    void write_jpg( const std :: string& filename, int quality );
    void write_png( const std :: string& filename );
};

#endif // __FIMAGE_HPP