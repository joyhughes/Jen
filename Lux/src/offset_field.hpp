#ifndef __OFFSET_FIELD_HPP
#define __OFFSET_FIELD_HPP

#include "image.hpp"

class offset_field : public image< vec2i > {

public:
    offset_field() : image() {}     
    // creates image of particular size 
    offset_field( const vec2i& dims ) : image( dims ){}     
    offset_field( const vec2i& dims, const bb2f& bb ) : image( dims, bb ) {}     
    // copy constructor
    offset_field( const image< vec2i >& img ) : image( img ) {}  
    offset_field( const image< vec2f >& vfield, const bool relative = false, const image_extend extend = SAMP_REPEAT ) { fill( vfield, relative ); }  
    
    // fill warp field values based on vector field
    void fill( const image< vec2f >& vfield, const bool relative = false, const image_extend extend = SAMP_REPEAT );

    // necessary?
    // advect a value through the offset field
    //template< class T > inline T advect( unsigned int index, image< T >& img ) 
    //    { img.set( index, img.index[ base[ index ] ] ); }
};

#endif // __OFFSET_FIELD_HPP
