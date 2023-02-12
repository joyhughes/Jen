#include "offset_field.hpp"

// fill warp field values based on vector field - fields should be same size
void offset_field::fill(const image< vec2f >& vfield, const bool relative, const image_extend extend ) {
    if( vfield.get_dim() != dim ) {
        std::cerr << "Error: vector field and warp field must be same size" << std::endl;
        return;
    }
    for( int x = 0; x < dim.x; x++ ) {
        for( int y = 0; y < dim.y; y++ ) {
            vec2f vf = vfield.index( y * dim.x + x );
            vf = fpbounds.bb_map( vf, vfield.get_bounds() ); 
            vec2i vi( vf + 0.5f );
            if( !relative ) { vi.x += x; vi.y += y; }
            if( extend == SAMP_SINGLE ) {
                if( ipbounds.in_bounds( vi ) ) base[ y * dim.x + x ] = vi;
                else if( relative ) base[ y * dim.x + x ] = { 0, 0 };
                else base[ y * dim.x + x ] = { x, y };
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
                base[ y * dim.x + x ] = { outx, outy };
            }
        }
    }
}