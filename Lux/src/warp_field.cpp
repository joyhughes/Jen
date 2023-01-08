#include "warp_field.hpp"

// fill warp field values based on vector field - fields should be same size
void warp_field::fill(const vector_field &vfield, bool relative ) {
    if( vfield.get_dim() != dim ) {
        std::cerr << "Error: vector field and warp field must be same size" << std::endl;
        return;
    }
    for( int x = 0; x < dim.x; x++ ) {
        for( int y = 0; y < dim.y; y++ ) {
            vec2f vi = vfield.index( y * dim.x + x ); 
            vi.x += 0.5f; vi.y += 0.5f;
            int xblock = vi.x / dim.x;  if( vi.x < 0 ) { xblock -= 1; }
            int yblock = vi.y / dim.y;  if( vi.y < 0 ) { yblock -= 1; }
            int outx = vi.x - ( xblock * dim.x );
            int outy = vi.y - ( yblock * dim.y );

            base[ y * dim.x + x ] = outx + outy * dim.x;
        }
    }
}


