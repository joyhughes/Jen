// Floating point image class using image template as a base

#ifndef __FIMAGE_HPP
#define __FIMAGE_HPP

#include "image.hpp"

class fimage : public image< frgb > {

protected:
    void quantize( std :: vector< unsigned char >& img );

public:
    fimage() : image< frgb >() {}      
    fimage( const vec2i& dims ) : image< frgb >( dims ){}   // creates image of particular size
    fimage( const vec2i& dims, const bb2f& bb ) : image< frgb >( dims, bb ) {}        
    fimage( const I& img ) : image< frgb >( img ) {}        // copy constructor
    fimage( const std::string& filename ) : image< frgb >() { load( filename ); } 

    // pixel modification functions
    void clamp( float minc = 0.0f, float maxc = 1.0f );
    void constrain();
    void grayscale();

    // I/O functions
    void load(      const std::string& filename );
    void write_jpg( const std::string& filename, int quality );
    void write_png( const std::string& filename );
};

#endif // __FIMAGE_HPP