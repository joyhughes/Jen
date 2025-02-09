// Template base class for rasterized data - used as base for image classes and vector fields
// Handles functions common to all of these classes.
// Supports linear interpolated sampling and mip-mapping - multiple resolutions for anti-aliasing

#include "image.hpp"
#include "fimage.hpp"
#include "uimage.hpp"
#include "vector_field.hpp"
#include "warp_field.hpp"
#include "any_image.hpp"
#include <iostream>
#include <fstream>

// Inline functions only used in image.cpp

template< class T > inline T blend4( const T& a, const T& b, const T& c, const T& d ) {
    return ( a + b + c + d ) / 4;
}

template<> inline ucolor blend4( const ucolor& a, const ucolor& b, const ucolor& c, const ucolor& d ) {
    return blend(a, b, c, d);
}

template< class T > inline T blend_tent( const T& center, const T& edge1, const T& edge2, const T& edge3, const T& edge4, const T& corner1, const T& corner2, const T& corner3, const T& corner4 ) {
    return ( center * 4 + ( edge1 + edge2 + edge3 + edge4 ) * 2 + corner1 + corner2 + corner3 + corner4 ) / 16;
}

template<> inline ucolor blend_tent( const ucolor& center, const ucolor& edge1, const ucolor& edge2, const ucolor& edge3, const ucolor& edge4, const ucolor& corner1, const ucolor& corner2, const ucolor& corner3, const ucolor& corner4 ) {
   return
   ( ( ( ( ( ( ( center & 0x00ff0000 ) << 2 ) + ( ( ( edge1 & 0x00ff0000 ) + ( edge2 & 0x00ff0000 ) + ( edge3 & 0x00ff0000 ) + ( edge4 & 0x00ff0000 ) ) << 1 ) + ( corner1 & 0x00ff0000 ) + ( corner2 & 0x00ff0000 ) + ( corner3 & 0x00ff0000 ) + ( corner4 & 0x00ff0000 ) ) ) + 0x00080000 ) >> 4 ) & 0x00ff0000 ) + 
   ( ( ( ( ( ( ( center & 0x0000ff00 ) << 2 ) + ( ( ( edge1 & 0x0000ff00 ) + ( edge2 & 0x0000ff00 ) + ( edge3 & 0x0000ff00 ) + ( edge4 & 0x0000ff00 ) ) << 1 ) + ( corner1 & 0x0000ff00 ) + ( corner2 & 0x0000ff00 ) + ( corner3 & 0x0000ff00 ) + ( corner4 & 0x0000ff00 ) ) ) + 0x00000800 ) >> 4 ) & 0x0000ff00 ) + 
   ( ( ( ( ( ( ( center & 0x000000ff ) << 2 ) + ( ( ( edge1 & 0x000000ff ) + ( edge2 & 0x000000ff ) + ( edge3 & 0x000000ff ) + ( edge4 & 0x000000ff ) ) << 1 ) + ( corner1 & 0x000000ff ) + ( corner2 & 0x000000ff ) + ( corner3 & 0x000000ff ) + ( corner4 & 0x000000ff ) ) ) + 0x00000008 ) >> 4 ) & 0x000000ff ) + 
   0xff000000; // blend alphas?
}

// edge case - center, three edges, two corners
template< class T > inline T blend_tent_edge( const T& center, const T& edge1, const T& edge2, const T& edge3, const T& corner1, const T& corner2 ) {
    return ( center * 4 + ( edge1 + edge2 + edge3 ) * 2 + corner1 + corner2 ) / 12;
}

template<> inline ucolor blend_tent_edge( const ucolor& center, const ucolor& edge1, const ucolor& edge2, const ucolor& edge3, const ucolor& corner1, const ucolor& corner2 ) {
   return
   ( ( ( ( ( ( ( center & 0x00ff0000 ) << 2 ) + ( ( ( edge1 & 0x00ff0000 ) + ( edge2 & 0x00ff0000 ) + ( edge3 & 0x00ff0000 ) ) << 1 ) + ( corner1 & 0x00ff0000 ) + ( corner2 & 0x00ff0000 ) ) >> 2 ) + 0x00060000 ) / 12 ) & 0x00ff0000 ) + 
   ( ( ( ( ( ( ( center & 0x0000ff00 ) << 2 ) + ( ( ( edge1 & 0x0000ff00 ) + ( edge2 & 0x0000ff00 ) + ( edge3 & 0x0000ff00 ) ) << 1 ) + ( corner1 & 0x0000ff00 ) + ( corner2 & 0x0000ff00 ) ) >> 2 ) + 0x00000600 ) / 12 ) & 0x0000ff00 ) + 
   ( ( ( ( ( ( ( center & 0x000000ff ) << 2 ) + ( ( ( edge1 & 0x000000ff ) + ( edge2 & 0x000000ff ) + ( edge3 & 0x000000ff ) ) << 1 ) + ( corner1 & 0x000000ff ) + ( corner2 & 0x000000ff ) ) >> 2 ) + 0x00000006 ) / 12 ) & 0x000000ff ) + 
   0xff000000; // blend alphas?
}

// corner case - center, two edges, one corner
template< class T > inline T blend_tent_corner( const T& center, const T& edge1, const T& edge2, const T& corner1 ) {
    return ( center * 4 + ( edge1 + edge2 ) * 2 + corner1 ) / 9;
}

template<> inline ucolor blend_tent_corner( const ucolor& center, const ucolor& edge1, const ucolor& edge2, const ucolor& corner1 ) {
    unsigned int random_bit = fair_coin( gen );
    return
    ( ( ( ( ( ( ( center & 0x00ff0000 ) << 2 ) + ( ( ( edge1 & 0x00ff0000 ) + ( edge2 & 0x00ff0000 ) ) << 1 ) + ( corner1 & 0x00ff0000 ) ) >> 2 ) + 0x00040000 + random_bit ) / 9 ) & 0x00ff0000 ) + 
    ( ( ( ( ( ( ( center & 0x0000ff00 ) << 2 ) + ( ( ( edge1 & 0x0000ff00 ) + ( edge2 & 0x0000ff00 ) ) << 1 ) + ( corner1 & 0x0000ff00 ) ) >> 2 ) + 0x00000400 + random_bit ) / 9 ) & 0x0000ff00 ) + 
    ( ( ( ( ( ( ( center & 0x000000ff ) << 2 ) + ( ( ( edge1 & 0x000000ff ) + ( edge2 & 0x000000ff ) ) << 1 ) + ( corner1 & 0x000000ff ) ) >> 2 ) + 0x00000004 + random_bit ) / 9 ) & 0x000000ff ) + 
    0xff000000; // blend alphas?
}

#define MIP(   xm, ym ) mip[ level     ][ ( ym ) * mip_dim[ level     ].x + ( xm ) ]
#define BELOW( xb, yb ) mip[ level - 1 ][ ( yb ) * mip_dim[ level - 1 ].x + ( xb ) ]

template< class T > void image< T >::mip_it() { // mip it good
    kernel = MIP_TENT;
    std::cout << "mip it" << std::endl;
    if( mip_me ) {
        std::cout << "mip it good" << std::endl;
        if( !mipped ) {
            std::cout << "mip_it: mipped = false" << std::endl;
            if( mip.size() > 1 ) {
                std::cout << "warning: mip_it: mip.size() > 1"  << std::endl;
                de_mip();
            }            
            if( mip.size() == 0 ) {
                std::cout << "warning: mip_it: mip.size() == 0"  << std::endl;
                de_mip();
            }
            int level = 0;
            mip_dim.push_back( dim );
            // mip[0] should already contain base data
            // allocate mip map memory
            while( mip_dim[ level ].x > 1 || mip_dim[ level ].y > 1 ) {
                //if( mip.size() <= level + 1 ) {
                    mip.push_back( std::vector< T >( ( mip_dim[ level ].x + 1 ) / 2 * ( mip_dim[ level ].y + 1 ) / 2 ) );
                    mip_dim.push_back( vec2i( ( mip_dim[ level ].x + 1 ) / 2, ( mip_dim[ level ].y + 1 ) / 2 ) );
                //}
                level++;
            }
            mipped = true;
        }
        if( !mip_utd ) {
            std::cout << "mip_it: mip_utd = false" << std::endl;
            unsigned int mip_level, mip_blend;
            // calculate mip-maps
            if( kernel == MIP_BOX ) {
                int maxx, maxy;
                for( int level = 1; level < mip.size(); level++ ) {
                    vec2i& m = mip_dim[ level ];
                    vec2i& b = mip_dim[ level - 1 ];
                    if( b.x % 2 ) maxx = m.x - 1; else maxx = m.x;
                    if( b.y % 2 ) maxy = m.y - 1; else maxy = m.y;
                    for( int y = 0; y < maxy; y++ ) {
                        for( int x = 0; x < maxx; x++ ) {
                            // central part of image
                            MIP( x, y ) = blend4< T >( 
                                BELOW( x * 2,     y * 2 ),
                                BELOW( x * 2 + 1, y * 2 ),
                                BELOW( x * 2,     y * 2 + 1 ),
                                BELOW( x * 2 + 1, y * 2 + 1 )
                            );
                        }
                        if( b.x % 2 ) {
                            // right edge
                            MIP( m.x - 1, y ) = blend( 
                                BELOW( b.x - 1, y * 2     ),
                                BELOW( b.x - 1, y * 2 + 1 )
                            );
                        }
                    }
                    if( b.y % 2 ) {
                        for( int x = 0; x < maxx; x++ ) {
                            // bottom edge
                            MIP( x, m.y - 1 ) = blend(
                                BELOW( x * 2,     b.y - 1 ),
                                BELOW( x * 2 + 1, b.y - 1 )
                            );
                        }
                        if( b.x % 2 ) {
                            // lower right corner
                            MIP( m.x - 1, m.y - 1 ) = BELOW( b.x - 1, b.y - 1 );
                        }
                    }
                }
            }
            else if( kernel == MIP_TENT ) {
                int maxx, maxy;
                for( int level = 1; level < mip.size(); level++ ) {
                    vec2i& m = mip_dim[ level ];
                    vec2i& b = mip_dim[ level - 1 ];
                    // handle small mip-maps
                    if( b.x % 2 ) maxx = m.x - 1; else maxx = m.x;
                    if( b.y % 2 ) maxy = m.y - 1; else maxy = m.y;
                    // upper left corner
                    MIP( 0, 0 ) = blend_tent_corner( BELOW( 0, 0 ), BELOW( 0, 1 ), BELOW( 1, 0 ), BELOW( 1, 1 ) );
                    // upper right corner
                    if( b.x % 2 ) MIP( m.x - 1, 0 ) = blend_tent_corner( BELOW( b.x - 1, 0 ), BELOW( b.x - 2, 0 ), BELOW( b.x - 1, 1 ), BELOW( b.x - 2, 1 ) );
                    if( b.y % 2 ) {
                        // lower left corner
                        MIP( 0, m.y - 1 ) = blend_tent_corner( BELOW( 0, b.y - 1 ), BELOW( 0, b.y - 2 ), BELOW( 1, b.y - 1 ), BELOW( 1, b.y - 2 ) );
                        if( mip_dim[ level - 1 ].x % 2 ) {
                            // lower right corner
                            MIP( m.x - 1, m.y - 1 ) = blend_tent_corner( BELOW( b.x - 1, b.y - 1 ), BELOW( b.x - 2, b.y - 1 ), BELOW( b.x - 1, b.y - 2 ), BELOW( b.x - 2, b.y - 2 ) );
                        }
                    }
                    // upper edge
                    for( int x = 1; x < maxx; x++ ) {
                        MIP( x, 0 ) = blend_tent_edge( 
                            BELOW( x * 2,     0 ),
                            BELOW( x * 2 - 1, 0 ),
                            BELOW( x * 2 + 1, 0 ),
                            BELOW( x * 2,     1 ),
                            BELOW( x * 2 - 1, 1 ),
                            BELOW( x * 2 + 1, 1 )
                        );
                    }
                    for( int y = 1; y < maxy; y++ ) {
                        // left edge
                        MIP( 0, y ) = blend_tent_edge( 
                            BELOW( 0, y * 2 ),
                            BELOW( 0, y * 2 - 1 ),
                            BELOW( 0, y * 2 + 1 ),
                            BELOW( 1, y * 2 ),
                            BELOW( 1, y * 2 - 1 ),
                            BELOW( 1, y * 2 + 1 )
                        );
                        for( int x = 1; x < maxx; x++ ) {
                            // central part of image
                            MIP( x, y ) = blend_tent( 
                                BELOW( x * 2,     y * 2 ),
                                BELOW( x * 2 - 1, y * 2 ),
                                BELOW( x * 2 + 1, y * 2 ),
                                BELOW( x * 2,     y * 2 - 1 ),
                                BELOW( x * 2,     y * 2 + 1 ),
                                BELOW( x * 2 - 1, y * 2 - 1 ),
                                BELOW( x * 2 + 1, y * 2 - 1 ),
                                BELOW( x * 2 - 1, y * 2 + 1 ),
                                BELOW( x * 2 + 1, y * 2 + 1 )
                            );
                        }
                        if( b.x % 2 ) {
                            // right edge
                            MIP( m.x - 1, y ) = blend_tent_edge( 
                                BELOW( b.x - 1, y * 2 ),
                                BELOW( b.x - 2, y * 2 ),
                                BELOW( b.x - 1, y * 2 - 1 ),
                                BELOW( b.x - 1, y * 2 + 1 ),
                                BELOW( b.x - 2, y * 2 - 1 ),
                                BELOW( b.x - 2, y * 2 + 1 )
                            );
                        }
                    }
                    if( b.y % 2 ) {
                        // bottom edge
                        for( int x = 1; x < maxx; x++ ) {
                            MIP( x, m.y - 1 ) = blend_tent_edge( 
                                BELOW( x * 2,     b.y - 1 ),
                                BELOW( x * 2 - 1, b.y - 1 ),
                                BELOW( x * 2 + 1, b.y - 1 ),
                                BELOW( x * 2,     b.y - 2 ),
                                BELOW( x * 2 - 1, b.y - 2 ),
                                BELOW( x * 2 + 1, b.y - 2 )
                            );
                        }
                    }
                }
            }
            mip_utd = true;
        }
    }
}

template< class T > void image< T >::de_mip() {  
    mipped = false;
    mip_utd = false;
    // deallocate all mip-maps except base
    // remove all elements of mip vector except first
    if( mip.size() > 1 ) mip.erase( mip.begin() + 1, mip.end() );
    std::cout << "de_mip()" << std::endl;
    mip_dim.clear();
}

template< class T > void image< T >::reset() { 
    std::cout << "image::reset()" << std::endl;
    set_dim( { 0, 0 } );
    de_mip(); 
}

template< class T > void image< T >::use_mip( bool m ) {
    mip_me = m;
    /*
    if( mip_me ) mip_it();
    else de_mip();
    */
}

template< class T > const vec2i image< T >::get_dim() const { return dim; }

// Reallocates base memory to match new dimensions, if needed
template< class T > void image< T >::set_dim( const vec2i& dims ) {
    //std::cout << "image::set_dim()" << std::endl;
    if( dim != dims ) {
        auto& base = mip[ 0 ];
        base.resize( dims.x * dims.y );
        de_mip();
        dim = dims;
        refresh_bounds();
        //mip_it();
    }
}

// calculates default bounding boxes based on pixel dimensions
template< class T > void image< T >::refresh_bounds() { 
    bounds.set( { -1.0f, ( 1.0f * dim.y ) / dim.x }, { 1.0f, ( -1.0f * dim.y ) / dim.x } );
    ipbounds.set( { 0, 0 }, dim );
    fpbounds.set( { 0.0f, 0.0f }, ( vec2f )dim - 1.0f );
}

template< class T > const bb2f image< T >::get_bounds() const { return bounds; }
template< class T > const bb2i image< T >::get_ipbounds() const { return ipbounds; }
template< class T > const bb2f image< T >::get_fpbounds() const { return fpbounds; }
template< class T > void image< T >::set_bounds( const bounding_box< float, 2 >& bb ) { bounds = bb; }

// returns true if images have same dimensions

template< class T > const T image< T >::index ( const vec2i& vi, const image_extend& extend ) const {

    T result;   // expect zero initialization
    auto& base = mip[ 0 ];

    if( extend == SAMP_SINGLE ) {
        if( ipbounds.in_bounds_half_open( vi ) ) result = base[ vi.y * dim.x + vi.x ]; // else retain zero-initialized result
    }
    else {
        int xblock = vi.x / dim.x;  if( vi.x < 0 ) { xblock -= 1; }
        int yblock = vi.y / dim.y;  if( vi.y < 0 ) { yblock -= 1; }
        int x = vi.x - ( xblock * dim.x );
        int y = vi.y - ( yblock * dim.y );
        if( extend == SAMP_REFLECT ) {
            if( xblock % 2 ) 	{ x = dim.x - 1 - x; }
            if( yblock % 2 ) 	{ y = dim.y - 1 - y; }
        }
        result = base[ y * dim.x + x ];
    }
    return result;
}
/*
template< class T > const T image< T >::index ( const int& level, const vec2i& vi, const image_extend& extend ) const {

    T result;   // expect zero initialization
    const vec2i d = mip_dim[ level ];

    if( mipped ) {
        if( level >=0 && level < mip.size() ) {          
            if( extend == SAMP_SINGLE ) {
                if( ipbounds.in_bounds_half_open( vi ) ) result = mip[level]->at( vi.y * d.x + vi.x ); // else retain zero-initialized result
            }
            else {
                int xblock = vi.x / d.x;  if( vi.x < 0 ) { xblock -= 1; }
                int yblock = vi.y / d.y;  if( vi.y < 0 ) { yblock -= 1; }
                int x = vi.x - ( xblock * d.x );
                int y = vi.y - ( yblock * d.y );
                if( extend == SAMP_REFLECT ) {
                    if( xblock % 2 ) 	{ x = d.x - 1 - x; }
                    if( yblock % 2 ) 	{ y = d.y - 1 - y; }
                }
                result = mip[level]->at( y * d.x + x );
            }
        }
    }
    return result;
}
*/
// interesting bug preserved in amber
template< class T > const T image< T >::sample_tile ( const vec2f& v, const bool& smooth, const image_extend& extend ) const {
    using namespace linalg;

    vec2i vi = ipbounds.bb_map( v, bounds );
    if( v.x < 0.0f ) vi.x -= 1;     // Correct for round towards zero
    if( v.y < 0.0f ) vi.y -= 1;
    if( smooth ) {  // linearly interpolated between neighboring values
        vec2f rem = v - bounds.bb_map( vi, ipbounds );  // Get remainders for interpolation
        return blendf( blendf ( index( vi ,               extend ), index( { vi.x + 1, vi.y     }, extend ), rem.x ),
                      blendf ( index( { vi.x, vi.y + 1}, extend ), index( { vi.x + 1, vi.y + 1 }, extend ), rem.x ), rem.y );
 //       return lerp( lerp ( index( { vi.x + 1, vi.y + 1 }, extend ), index( { vi.x, vi.y + 1}, extend ), rem.x ), 
 //                    lerp ( index( { vi.x + 1, vi.y     }, extend ), index( vi ,               extend ), rem.x ), rem.y );
    }
    else return( index( vi , extend ) ); // quick and dirty sampling of nearest pixel value
}

template< class T > const T image< T >::sample ( const vec2f& v, const bool& smooth, const image_extend& extend ) const {
    using namespace linalg;
    std::cout << "why am I here?" << std::endl;
    vec2f vf = fpbounds.bb_map( v, bounds );
    vec2i vi = ( vec2i )vf;
    if( vf.x < 0.0f ) vi.x -= 1;     // Correct for round towards zero
    if( vf.y < 0.0f ) vi.y -= 1;
    if( smooth ) {  // linearly interpolated between neighboring values
        vec2f rem = vf - ( vec2f )vi;  // Get remainders for interpolation
        return blendf( blendf ( index( vi ,               extend ), index( { vi.x + 1, vi.y     }, extend ), rem.x ),
                      blendf ( index( { vi.x, vi.y + 1}, extend ), index( { vi.x + 1, vi.y + 1 }, extend ), rem.x ), rem.y );
 //       return lerp( lerp ( index( { vi.x + 1, vi.y + 1 }, extend ), index( { vi.x, vi.y + 1}, extend ), rem.x ), 
 //                    lerp ( index( { vi.x + 1, vi.y     }, extend ), index( vi ,               extend ), rem.x ), rem.y );
    }
    else return( index( vi, extend ) ); // quick and dirty sampling of nearest pixel value
}

// Fixed point version of sample

template< class T > const T image< T >::sample ( const unsigned int& mip_level, const unsigned int& mip_blend, const vec2i& vi ) const  {
    int l_index = ( vi.x >> ( 16 + mip_level     ) ) + ( vi.y >> ( 16 + mip_level     ) ) * mip_dim[ mip_level     ].x;
    int u_index = ( vi.x >> ( 16 + mip_level + 1 ) ) + ( vi.y >> ( 16 + mip_level + 1 ) ) * mip_dim[ mip_level + 1 ].x;
    return  blendf(
                blendf(
                    blendf( mip[ mip_level + 1 ][ u_index + mip_dim[ mip_level + 1 ].x + 1 ], mip[ mip_level + 1 ][ u_index + mip_dim[ mip_level + 1 ].x ], ( ( vi.x >> ( mip_level + 1 ) ) & 0xffff ) / 65536.0f ),
                    blendf( mip[ mip_level + 1 ][ u_index                              + 1 ], mip[ mip_level + 1 ][ u_index                              ], ( ( vi.x >> ( mip_level + 1 ) ) & 0xffff ) / 65536.0f ),
                    ( ( vi.y >> mip_level ) & 0xffff ) / 65536.0f 
                ),
                blendf(
                    blendf( mip[ mip_level ][ l_index + mip_dim[ mip_level ].x + 1 ], mip[ mip_level ][ l_index + mip_dim[ mip_level ].x ], ( ( vi.x >> mip_level ) & 0xffff ) / 65536.0f ),
                    blendf( mip[ mip_level ][ l_index                          + 1 ], mip[ mip_level ][ l_index                          ], ( ( vi.x >> mip_level ) & 0xffff ) / 65536.0f ),
                    ( ( vi.y >> ( mip_level ) ) & 0xffff ) / 65536.0f 
                ),
                mip_blend / 65536.0f
            );
}

// Sets to background color everything outside of a centered circle
template< class T > void image< T >::crop_circle( const T& background, const float& ramp_width ) {
    float r2;   // radius in pixel space
    if( dim.x > dim.y ) r2 = dim.x / 2.0; 
    else r2 = dim.y / 2.0;
    float r1 = r2 * ( 1.0f - ramp_width );
    r1 = r1 * r1; r2 = r2 * r2;
    vec2f center = fpbounds.center();
    auto& base = mip[ 0 ];
    
    for( int x = 0; x < dim.x; x++ ) {
        for( int y = 0; y < dim.y; y++ ) {
            float r = linalg::length2( vec2f( { x * 1.0f, y * 1.0f } ) - center );
            if( r > r2 ) base[ y * dim.x + x ] = background;
            else blendf( base[ y * dim.x + x ], background, ( 1.0f - sqrtf( r / r2 ) ) / ramp_width );
        }
    }
    mip_utd = false; 
}

// Colors black everything outside of a centered circle
/*
template< class T > void image< T >::crop_circle( const float& ramp_width ) {
    crop_circle( b, ramp_width );
}
*/

template< class T > void image< T >::mirror(    const image< T >& in,
                                                const bool& reflect_x, 
                                                const bool& reflect_y, 
                                                const bool& top_to_bottom, 
                                                const bool& left_to_right, 
                                                const vec2f& center, 
                                                const image_extend& extend ) {
    if( in.dim != dim ) throw std::runtime_error( "mirror: input image must have same dimensions" );                                               
    vec2i icenter = ipbounds.bb_map( center, bounds );

    auto it = begin();
    for( int y = 0; y < dim.y; y++ ) {
        vec2i ip = { 0, y };
        if( reflect_y ) {
            if( !top_to_bottom ) 
                 { if( y < icenter.y ) ip.y = icenter.y + ( y - icenter.y ); }   
            else { if( y > icenter.y ) ip.y = icenter.y - ( y - icenter.y ); }
        }
        for( int x = 0; x < dim.x; x ++ ) {
            ip.x = x;
            if( reflect_x ) {
                if( left_to_right ) 
                     { if( x > icenter.x ) ip.x = icenter.x - ( x - icenter.x ); }
                else { if( x < icenter.x ) ip.x = icenter.x + ( x - icenter.x ); }
            }
            *it = in.index( ip, extend );
            it++;
        }
    }
    mip_utd = false;
}

template< class T > void image< T >::turn( const image< T >& in, const direction4& direction ) {
    if( direction == D4_UP || direction == D4_DOWN ) {
        if( in.dim != dim ) throw std::runtime_error( "turn: image size mismatch\n" ); 
        if( direction == D4_UP ) { std::copy( in.begin(), in.end(), begin() ); }
        else             { std::reverse_copy( in.begin(), in.end(), begin() ); }
    }
    else { // left or right
        if( in.dim.x != dim.y || in.dim.y != dim.x ) throw std::runtime_error( "turn: input image must have same dimensions, rotated 90 degrees\n" );
        auto it = begin();
        for( int y = 0; y < dim.y; y++ ) {
            for( int x = 0; x < dim.x; x++ ) {
                if( direction == D4_RIGHT ) *it = in.index( { dim.x - 1 - y, x } );
                else                        *it = in.index( { y, dim.y - 1 - x } );
                *it++;
            }
        }
    }
    mip_utd = false;
}

template< class T > void image< T >::flip( const image< T >& in, const bool& flip_x, const bool& flip_y ) {
    if( in.dim != dim ) throw std::runtime_error( "flip: image size mismatch" ); 
    auto it = begin();
    if( flip_x ) {
        if( flip_y ) { std::reverse_copy( in.begin(), in.end(), begin() ); }
        else {    
            auto in_it = in.begin(); auto it = begin();
            for( int y = 0; y < dim.y; y++ ) {
                std::reverse_copy( in_it, in_it + dim.x - 1, it );
                in_it += dim.x; it += dim.x;
            }
        }
    }
    else {
        if( flip_y ) {
            auto in_it = in.begin() + (dim.y - 1 ) * dim.x; auto it = begin();
            for( int y = 0; y < dim.y; y++ ) {
                std::copy( in_it, in_it + dim.x - 1, it );
                in_it -= dim.x; it += dim.x;
            }
        }
        else { std::copy( in.begin(), in.end(), begin() ); } // no change
    }
    mip_utd = false;
}

// copy image in place
template< class T > void image< T >::copy( const image< T >& img ) {
    //std::cout << "image::copy()" << std::endl;
    if( img.dim == dim ) {
        mip.assign( img.mip.begin(), img.mip.end() );
        mip_dim.assign( img.mip_dim.begin(), img.mip_dim.end() );
    } else {
        dim = img.dim;
        mip = img.mip;
        mip_dim = img.mip_dim;
    }  
    bounds = img.bounds;
    ipbounds = img.ipbounds;
    fpbounds = img.fpbounds;
    mip_me = img.mip_me;
    mipped = img.mipped;
    mip_utd = img.mip_utd;
    kernel = img.kernel;
}

template< class T > void image< T >::fill( const T& c ) {
    auto& base = mip[ 0 ];
    std::fill( begin(), base.end(), c );
    mip_utd = false;
}

template< class T > void image< T >::fill( const T& c, const bb2i& bb ) {
    auto& base = mip[ 0 ];
    if( ( bb.minv == ipbounds.minv ) && ( bb.maxv == ipbounds.maxv ) ) fill( c );
    else {
        auto bb1 = bb.intersect( ipbounds );
        for( int y = bb1.minv.y; y < bb1.maxv.y; y++ ) {
            // half open interval
            auto beg_it = base.begin() + ( y * dim.x + bb1.minv.x );
            auto end_it = base.begin() + ( y * dim.x + bb1.maxv.x - 1);
            std::fill( beg_it, end_it, c );
        }
    }
    mip_utd = false;
}

template< class T > void image< T >::fill( const T& c, const bb2f& bb ) {
    fill( c, bb.map_box( bounds, ipbounds ) );
    mip_utd = false;
}

template< class T > void image< T >::clear() {
    fill( black< T > );
    mip_utd = false;
}

// Black and white noise
template< class T > void image< T >::noise( const float& a ) {
    auto& base = mip[ 0 ];
    for( auto& pix : base ) { pix = weighted_bit(a) ? white< T > : black< T >; }
    mip_utd = false;
}

template< class T > void image< T >::noise( const float& a, const bb2i& bb ) {
    auto& base = mip[ 0 ];
    if( ( bb.minv == ipbounds.minv ) && ( bb.maxv == ipbounds.maxv ) ) noise( a );
    else {
        auto bb1 = bb.intersect( ipbounds );
        for( int y = bb1.minv.y; y < bb1.maxv.y; y++ ) {
            for( int x = bb1.minv.x; x < bb1.maxv.x; x++ ) {
                base[ y * dim.x + x ] = weighted_bit( a ) ? white< T > : black< T >;
            }
        }
    }
    mip_utd = false;
}

template< class T > void image< T >::noise( const float& a, const bb2f& bb ) {
    noise( a, bb.map_box( bounds, ipbounds ) );
}

template< class T > void image< T >::checkerboard( const int& box_size, const T& c1, const T& c2 ) {
    auto& base = mip[ 0 ];
    for( int y = 0; y < dim.y; y++ ) {
        for( int x = 0; x < dim.x; x++ ) {
            if( ( x / box_size + y / box_size ) % 2 ) base[ y * dim.x + x ] = c1;
            else base[ y * dim.x + x ] = c2;
        }
    }
    mip_utd = false;
}

template< class T > void image< T >::apply_mask( const image< T >& layer, const image< T >& mask, const mask_mode& mmode ) {
    auto l = layer.begin();
    auto m = mask.begin();
    for ( auto r = begin(); r <= end(); ) {
        ::apply_mask( *r, *l, *m, mmode );
        r++; l++; m++;
    }
    //mip_it();
}

// stubs for template specialization
template< class T > void image< T >::grayscale() {}
template< class T > void image< T >::constrain() {}
template< class T > void image< T >::invert() {}
template< class T > void image< T >::rgb_to_hsv() {}
template< class T > void image< T >::hsv_to_rgb() {}
template< class T > void image< T >::rotate_hue( const float& h ) {}
template< class T > void image< T >::rotate_components( const int& r ) {}
template< class T > void image< T >::clamp( float minc, float maxc ) {}

template< class T > void image< T >::splat( 
    const image< T >& splat_image,      // image of the splat
    const bool& smooth, 			    // smooth splat?
    const vec2f& center, 			    // coordinates of splat center
    const float& scale, 			    // radius of splat
    const float& theta, 			    // rotation in degrees
    const std::optional< std::reference_wrapper< image< T > > > mask,  // optional mask image
    const std::optional< T >&     tint, // change the color of splat
    const mask_mode& mmode              // how will mask be applied to splat and backround?
)  
{  
    std::cout << "splat: smooth = " << smooth << std::endl;
    const image< T >& g( splat_image );
    T gval, mval;
    auto& base = mip[ 0 ];

    bool has_tint = tint.has_value();
    T my_tint;
    if( has_tint ) my_tint = *tint;
    bool has_mask = mask.has_value();

    float thrad = theta / 360.0 * TAU;              // theta in radians
    vec2i p = ipbounds.bb_map( center, bounds);     // center of splat in pixel coordinates
	int size = scale / ( bounds.b2.x - bounds.b1.x ) * dim.x; // scale in pixel coordinates
    bb2i sbounds( p, size );    // bounding box of splat
	vec2f smin = bounds.bb_map( sbounds.minv, ipbounds );
	vec2f sc;
	sc.x = (smin.x - center.x) / scale;
	sc.y = (smin.y - center.y) / scale;
    sc = linalg::rot( -thrad, sc );

	// calculate unit vectors - one pixel long
	vec2f unx, uny;
	unx.x = 1.0f / dim.x * ( bounds.b2.x - bounds.b1.x ) / scale;
	unx.y = 0.0f;
    unsigned int unit_scale = (unsigned int)( std::fabs( unx.x ) * g.dim.x / 2.0f * 65536.0f );
	unx = linalg::rot( -thrad, unx );
	uny.x = 0.0f;
	uny.y = -1.0f / dim.y * ( bounds.b2.y - bounds.b1.y ) / scale;
	uny = linalg::rot( -thrad, uny );

	// convert vectors to splat pixel space - fixed point
    vec2i scfix = ( vec2i )(( sc * g.dim / 2.0f + g.dim / 2.0f ) * 65536.0f );
    vec2i unxfix = ( vec2i )( unx * g.dim / 2.0f * 65536.0f );
    vec2i unyfix = ( vec2i )( uny * g.dim / 2.0f * 65536.0f );
	vec2i sfix;
    bb2i fixbounds( { 0, 0 }, { ( g.dim.x - 1 ) << 16, ( g.dim.y - 1 ) << 16 });

    // calculate mip level and blend constant of splat
    unsigned int mip_level, mip_blend;
    mip_level = 0;
    while( unit_scale >> ( mip_level + 16 ) ) mip_level++;  // level is log2 of unit_scale
    mip_blend = 0;
    if( mip_level > 0 ) {
        mip_blend = ( unit_scale >> mip_level ) & 0xffff;
        mip_level--; 
    }

    std::cout << "mip_level: " << mip_level << " mip_blend: " << mip_blend << std::endl;
    // *** Critical loop below ***
    // Quick and dirty sampling, high speed but risk of aliasing. 
    // Should work best if splat is fairly large and smooth.
    // future: add option to smooth sample into splat's mip-map
    // future: add vector and color effects
    if( has_mask ) {
        const image< T >& m = mask->get();
        std::cout << "g.mip.size() = " << g.mip.size() << std::endl;
        for( int level = 0; level < g.mip.size(); level++ ) {
            std::cout << "g.mip[" << level << "].size() = " << g.mip[ level ].size() << std::endl;
        }
        std::cout << "m.mip.size() = " << m.mip.size() << std::endl;
        for( int level = 0; level < m.mip.size(); level++ ) {
            std::cout << "m.mip[" << level << "].size() = " << m.mip[ level ].size() << std::endl;
        }
        // image and mask same size
        if( m.dim == g.dim ) {
            for( int x = sbounds.minv.x; x < sbounds.maxv.x; x++ ) {
                sfix = scfix;
                if( ( x >= 0 ) && ( x < dim.x ) ) {
                    for( int y = sbounds.minv.y; y < sbounds.maxv.y; y++ ) {
                        if( ( y >= 0 ) && ( y < dim.y ) && fixbounds.in_bounds( sfix ) ) {
                            if( smooth ) { 
                                std::cout << "smooth sample sfix.x = " << sfix.x << " sfix.y = " << sfix.y << std::endl; 
                                gval = g.sample( mip_level, mip_blend, sfix );
                                mval = m.sample( mip_level, mip_blend, sfix );
                            }
                            else {
                                gval = *( g.begin() + (sfix.y >> 16) * g.dim.x + (sfix.x >> 16) );
                                mval = *( m.begin() + (sfix.y >> 16) * m.dim.x + (sfix.x >> 16) );
                            }
                            if( has_tint ) ::apply_mask( base[ y * dim.x + x ], mulc( gval, my_tint ), mval, mmode );
                            else ::apply_mask( base[ y * dim.x + x ], gval, mval, mmode );
                        }                        
                        sfix += unyfix;
                    }
                }
                scfix += unxfix;
            }
        }
        else {        
            unsigned int m_mip_level, m_mip_blend;
            m_mip_level = mip_level;
            m_mip_blend = mip_blend;
            // if image and mask are different sizes, step through separately
            vec2i mscfix = ( vec2i )(( sc * m.dim / 2.0f + m.dim / 2.0f ) * 65536.0f );
            vec2i munxfix = ( vec2i )( unx * m.dim / 2.0f * 65536.0f );
            vec2i munyfix = ( vec2i )( uny * m.dim / 2.0f * 65536.0f );
            vec2i msfix;
            bb2i  mfixbounds( { 0, 0 }, { ( m.dim.x - 1 ) << 16, ( m.dim.y - 1 ) << 16 }); 
            for( int x = sbounds.minv.x; x < sbounds.maxv.x; x++ ) {
                sfix = scfix;
                msfix = mscfix;
                if( ( x >= 0 ) && ( x < dim.x ) ) {
                for( int y = sbounds.minv.y; y < sbounds.maxv.y; y++ ) {
                    if( ( y >= 0 ) && ( y < dim.y ) && fixbounds.in_bounds( sfix ) ) {
                            if( smooth ) { 
                                gval = g.sample( mip_level, mip_blend, sfix );
                                mval = m.sample( m_mip_level, m_mip_blend, msfix );
                            }
                            else {
                                gval = *( g.begin() + (sfix.y  >> 16) * g.dim.x + (sfix.x  >> 16) );
                                mval = *( m.begin() + (msfix.y >> 16) * m.dim.x + (msfix.x >> 16) );
                            }
                            if( has_tint ) ::apply_mask( base[ y * dim.x + x ], mulc( gval, my_tint ), mval, mmode );
                            else           ::apply_mask( base[ y * dim.x + x ], gval, mval, mmode );
                        }
                        sfix  +=  unyfix;
                        msfix += munyfix;
                    }
                }
                scfix += unxfix;
                mscfix += munxfix;
            }
        }      
    }
    else {
        for( int x = sbounds.minv.x; x < sbounds.maxv.x; x++ ) {
            sfix = scfix;
            if( ( x >= 0 ) && ( x < dim.x ) ) {
                for( int y = sbounds.minv.y; y < sbounds.maxv.y; y++ ) {
                    if( ( y >= 0 ) && ( y < dim.y ) && fixbounds.in_bounds( sfix ) ) {
                        if( smooth ) gval = g.sample( mip_level, mip_blend, sfix );
                        else gval = *( g.begin() + (sfix.y >> 16) * g.dim.x + (sfix.x >> 16) );
                        if( has_tint ) addc( base[ y * dim.x + x ], mulc( gval, my_tint ) );
                        else           addc( base[ y * dim.x + x ], gval );
                    }
                    sfix += unyfix;
                }
            }
            scfix += unxfix;
        }
    }
    mip_utd = false;
}

template< class T > void image< T >::warp (  const image< T >& in, 
                                    const image< vec2f >& vf, 
                                    const float& step,            // default 1.0
                                    const bool& smooth,           // default false
                                    const bool& relative,         // default true
                                    const image_extend& extend )  // default SAMP_SINGLE 
{
    bool same_dims = compare_dims( vf ); // If vector field and input image are same dimension, interpolation not necessary
    if( ( !relative ) && same_dims )
        std::transform( begin(), end(), vf.begin(), begin(), [ &in, &smooth, &extend ] ( T &r, const vec2f &v ) { return in.sample( v, smooth, extend ); } );
    else {
        auto it = begin();
        auto vfit = vf.begin();
        vec2f v, coord;
        bb2f vf_bounds = vf.get_bounds();
        for( int y = 0; y < dim.y; y++ ) {
            for( int x = 0; x < dim.x; x++ ) {
                coord = bounds.bb_map( vec2i( x, y ), ipbounds );
                if( same_dims ) { v = *vfit; vfit++; }
                else { v = vf.sample( vf_bounds.bb_map( coord, bounds ), true ); }
                if( relative ) v = v * step + coord;
                *it = in.sample( v, smooth, extend );
                it++;
            }
        }
    }
    mip_utd = false;
}

template< class T > void image< T >::warp (  const image< T >& in, 
                                    const std::function< vec2f( vec2f ) >& vfn, 
                                    const float& step,            // default 1.0
                                    const bool& smooth,           // default false
                                    const bool& relative,         // default true
                                    const image_extend& extend )  // default SAMP_SINGLE 
{
    auto it = begin();
    for( int y = 0; y < dim.y; y++ ) {
        for( int x = 0; x < dim.x; x++ ) {
            vec2f coord = bounds.bb_map( vec2i( x, y ), ipbounds );
            vec2f v = vfn( coord );
            if( relative ) v = v * step + coord;
            *it = in.sample( v, smooth, extend );
            it++;
        }
    }
    mip_utd = false;
}

template< class T > void image< T >::warp ( const image< T >& in, 
                                            const image< int >& wf ) {
    if( !compare_dims( wf ) ) return; // Vector field and warp field must be same dimension
    if( !compare_dims( in ) ) return; // Vector field and input image must be same dimension
    std::transform( begin(), end(), wf.begin(), begin(), [ &in ] ( T &r, const unsigned int &i ) { return in.index( i ); } );
    mip_utd = false;
}

template< class T > void image< T >::warp(  const image< T > &in, 
                                            const image< vec2i > &of, 
                                            const vec2i &slide, 
                                            const image_extend &extend, 
                                            const image_extend &of_extend ) {
    
    auto warp_bounds = ipbounds;
    if( of_extend == SAMP_SINGLE ) warp_bounds.intersect( bb2i( slide, slide + of.get_dim() ) );

    vec2i v;
    for ( v.y = warp_bounds.minv.y; v.y < warp_bounds.maxv.y; v.y++) {
        auto it = begin() + v.y * dim.x + warp_bounds.minv.x;
        for ( v.x = warp_bounds.minv.x; v.x < warp_bounds.maxv.x; v.x++) {
            vec2i coord = of.index( v - slide, of_extend );
            *it = in.index( v + coord , extend );
            it++;
        }
    }
    mip_utd = false;
}

template< class T > void image< T >::read_binary(  const std::string &filename )
{
    vec2i new_dim;
    bb2f new_bounds;
    auto& base = mip[ 0 ];

    std::ifstream in_file( filename, std::ios::in | std::ios::binary );
    in_file.read( (char*)&new_dim, sizeof( vec2i ) );
    set_dim( new_dim );
    base.resize( dim.x * dim.y );

    in_file.read( (char*)&new_bounds, sizeof( bb2f ) );
    set_bounds( new_bounds );

    in_file.read( (char*)&(base[0]), dim.x * dim.y * sizeof( T ) );
    in_file.close();
    mip_utd = false;
}

template< class T > void image< T >::write_binary( const std::string &filename, int level )
{
    auto& pixels = mip[ level ];

    std::ofstream out_file( filename, std::ios::out | std::ios::binary );
    out_file.write( (char*)&dim, sizeof( vec2i ) );
    out_file.write( (char*)&bounds, sizeof( bb2f ) );
    out_file.write( (char*)&(pixels[0]), dim.x * dim.y * sizeof( T ) );
    out_file.close();
}

template< class T > void image< T >::write_file(const std::string &filename, file_type type, int quality, int level ) {
    if( level > 0 && !mipped) { std::cout << "image::write_file: mip-map not generated\n"; return; }
    if( level > mip.size() ) { std::cout << "image::write_file: mip-map level out of range\n"; return; }
    switch( type ) {
        case FILE_JPG: write_jpg( filename, quality, level ); break;
        case FILE_PNG: write_png( filename, level ); break;
        case FILE_BINARY: write_binary( filename, level ); break;
        default: std::cout << "image::write_file: unknown file type " << type << std::endl;
    }
}

// apply a vector function to each point in image
template< class T > void image< T >::apply( const std::function< T ( const T&, const float& ) > fn, const float& t ) {
    auto& base = mip[ 0 ];
    for( auto& v : base ) { v = fn( v, t ); }
    mip_utd = false;
} 

// copy assignment
template< class T > image< T >& image< T >::operator = ( const image< T >& rhs ) {
    if( this != &rhs ) {
        mip.resize( rhs.mip.size() );
        for( size_t i = 0; i < rhs.mip.size(); ++i ) {
            mip[ i ] = rhs.mip[ i ];
        }
        dim = rhs.dim;
        bounds = rhs.bounds;
        ipbounds = rhs.ipbounds;
        mip_me = rhs.mip_me;
        mip_utd = rhs.mip_utd;
        mipped = rhs.mipped;
    }
    return *this;
}

// move assignment
template< class T > image< T >& image< T >::operator = ( image< T >&& rhs ) {
    if( this != &rhs ) {
        mip = std::move( rhs.mip );
        dim = rhs.dim;
        bounds = rhs.bounds;
        ipbounds = rhs.ipbounds;
        mip_me = rhs.mip_me;
        mip_utd = rhs.mip_utd;
        mipped = rhs.mipped;
    }
    return *this;
}

template< class T > image< T >& image< T >::operator += ( image< T >& rhs ) {
    using namespace linalg;
    std::transform( begin(), end(), rhs.begin(), begin(), [] ( const T &a, const T &b ) { return a + b; } );
    mip_utd = false;
    return *this;
}

template< class T > image< T >& image< T >::operator += ( const T& rhs ) {
    using namespace linalg;
    std::transform( begin(), end(), begin(), [ rhs ]( const T &a ) { return a + rhs; } );
    mip_utd = false;
    return *this;
}

template< class T > image< T >& image< T >::operator -= ( image< T >& rhs ) {
    using namespace linalg;
    std::transform( begin(), end(), rhs.begin(), begin(), []( const T &a, const T &b ) { return a - b; } );
    mip_utd = false;
    return *this;
}

template< class T > image< T >& image< T >::operator -= ( const T& rhs ) {
    using namespace linalg;
    std::transform( begin(), end(), begin(), [ rhs ]( const T &a ) { return a - rhs; } );
    mip_utd = false;
    return *this;
}

template< class T > image< T >& image< T >::operator *= ( image< T >& rhs ) {
    using namespace linalg;
    std::transform( begin(), end(), rhs.begin(), begin(), [] ( const T &a, const T &b ) { return a * b; } );
    mip_utd = false;
    return *this;
}

template< class T > image< T >& image< T >::operator *= ( const T& rhs ) {
    using namespace linalg;
    std::transform( begin(), end(), begin(), [ rhs ]( const T &a ) { return a * rhs; } );
    mip_utd = false;
    return *this;
}

template< class T > image< T >& image< T >::operator *= ( const float& rhs ) {
    using namespace linalg;
    std::transform( begin(), end(), begin(), [ rhs ]( const T &a ) { return ( T )( a * rhs ); } );
    mip_utd = false;
    return *this;
}

template< class T > image< T >& image< T >::operator /= ( image< T >& rhs ) {
    using namespace linalg;
    std::transform( begin(), end(), rhs.begin(), begin(), [] ( const T &a, const T &b ) { return a / b; } );
    mip_utd = false;
    return *this;
}

template< class T > image< T >& image< T >::operator /= ( const T& rhs ) {
    using namespace linalg;
    std::transform( begin(), end(), begin(), [ rhs ]( const T &a ) { return a / rhs; } );
    mip_utd = false;
    return *this;
}

template< class T > image< T >& image< T >::operator /= ( const float& rhs ) {
    using namespace linalg;
    std::transform( begin(), end(), begin(), [ rhs ]( const T &a ) { return ( T )( a / rhs ); } );
    mip_utd = false;
    return *this;
}

template< class T > image< T >& image< T >::operator () () { return *this; }

template class image< frgb   >;      // fimage
template class image< ucolor >;      // uimage
template class image< vec2f  >;      // vector_field
//template class image< float >;     // scalar_field
template class image< int    >;      // warp_field
template class image< vec2i  >;      // offset_field 