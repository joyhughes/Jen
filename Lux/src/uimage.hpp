#ifndef __UIMAGE_HPP
#define __UIMAGE_HPP

#include "ucolor.hpp"
#include "image.hpp"

class uimage : public image< ucolor > {

protected:
    void spool( std :: vector< unsigned char >& img );

public:
    uimage() : image() {}     
    // creates image of particular size 
    uimage( const vec2i& dims ) : image( dims ){}     
    uimage( const vec2i& dims, const bb2f& bb ) : image( dims, bb ) {}     
    // copy constructor
    uimage( const I& img ) : image( img ) {}     
    // pixel modification functions
    void grayscale();

    // I/O functions
    void load(      const std :: string& filename );
    void write_jpg( const std :: string& filename, int quality );
    void write_png( const std :: string& filename );
};

#endif // __UIMAGE_HPP