#ifndef __OFFSET_FIELD_HPP
#define __OFFSET_FIELD_HPP

#include "image.hpp"

#define offset_field image< vec2i >

// fill offset field values based on vector field
template<> void offset_field::fill( const image< vec2f >& vfield, const bool relative, const image_extend extend );

/*template< class T > inline void advect( int index, image< T >& in, image< T > out ) // advect one pixel
    { out.set( index, in.index[ img.base[ index ] ] ); }
template< class T > void advect( image< T >& in, image< T >& out ) // advect entire image
    { for( int i = 0; i < img.base.size(); i++ ) advect( i, in, out ); }*/


#endif // __OFFSET_FIELD_HPP
