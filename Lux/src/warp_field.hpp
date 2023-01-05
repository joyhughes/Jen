#include "image.hpp"
#include "vector_field.hpp"

class warp_field : public image< unsigned int > {

public:
    warp_field() : image() {}     
    // creates image of particular size 
    warp_field( const vec2i& dims ) : image( dims ){}     
    warp_field( const vec2i& dims, const bb2f& bb ) : image( dims, bb ) {}     
    // copy constructor
    warp_field( const image< unsigned int >& img ) : image( img ) {}  
    warp_field( const vector_field& vfield, bool relative = false ) { fill( vfield, relative ); }  
    
    // fill warp field values based on vector field
    void fill( const vector_field& vfield, bool relative = false  );
    template< class T > inline T advect( unsigned int index, image< T >& img ) 
        { img.set( index, img.index[ base[ index ] ] ); }
};
