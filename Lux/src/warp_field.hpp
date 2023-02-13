#ifndef __WARP_FIELD_HPP
#define __WARP_FIELD_HPP

#include "image.hpp"

class warp_field : public image< int > {

public:
    warp_field() : image() {}     
    // creates image of particular size 
    warp_field( const vec2i& dims ) : image( dims ){}     
    warp_field( const vec2i& dims, const bb2f& bb ) : image( dims, bb ) {}     
    // copy constructor
    warp_field( const image< int >& img ) : image( img ) {}  
    warp_field( const image< vec2f >& vfield, const bool relative = false, const image_extend extend = SAMP_REPEAT ) { fill( vfield, relative ); }  
    
    // fill warp field values based on vector field
    void fill( const image< vec2f >& vfield, const bool relative = false, const image_extend extend = SAMP_REPEAT );
    template< class T > inline T advect( int index, image< T >& img ) 
        { img.set( index, img.index[ base[ index ] ] ); }
};

#endif // __WARP_FIELD_HPP