#include "warp_field.hpp"

// fill warp field values based on vector field - fields should be same size
template<> void warp_field::fill( const image< vec2f >& vfield, bool relative, const image_extend extend ) {
    for( int x = 0; x < dim.x; x++ ) {
        for( int y = 0; y < dim.y; y++ ) {
            vec2f vf = vfield.index( y * dim.x + x );
            vf = fpbounds.bb_map( vf, vfield.get_bounds() ); 
            vec2i vi( vf + 0.5f );
            if( !relative ) { vi.x += x; vi.y += y; }
            if( extend == SAMP_SINGLE ) {
                if( ipbounds.in_bounds( vi ) ) base[ y * dim.x + x ] = vi.y * dim.x + vi.x;
                else if( relative ) base[ y * dim.x + x ] = 0;
                else base[ y * dim.x + x ] = y * dim.x + x;
            }
            else {
                int xblock = vi.x / dim.x;  if( vi.x < 0 ) { xblock -= 1; }
                int yblock = vi.y / dim.y;  if( vi.y < 0 ) { yblock -= 1; }
                int outx = vi.x - ( xblock * dim.x );
                int outy = vi.y - ( yblock * dim.y );
                if( extend == SAMP_REFLECT ) {
                    if( xblock % 2 ) 	{ outx = dim.x - 1 - outx; }
                    if( yblock % 2 ) 	{ outy = dim.y - 1 - outy; }
                }
                base[ y * dim.x + x ] = outx + outy * dim.x;
            }
        }
    }
}
