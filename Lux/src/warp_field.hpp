#ifndef __WARP_FIELD_HPP
#define __WARP_FIELD_HPP

#include "image.hpp"

#define warp_field image< int >

// fill warp field values based on vector field
template<> void warp_field::fill( const image< vec2f >& vfield, const bool relative, const image_extend extend );
template<> template< class U > inline void warp_field::advect( int index, image< U >& in, image< U > out ) // advect one pixel
    { out.set( index, in.index[ base[ index ] ] ); }
template<> template< class U > void warp_field::advect( image< U >& in, image< U >& out ) // advect entire image
    { for( int i = 0; i < base.size(); i++ ) advect( i, in, out ); }

#endif // __WARP_FIELD_HPP
