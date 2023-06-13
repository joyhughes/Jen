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

template< class T > void image< T >::mip_it() { // mip it good
    if( mip_me ) {
        if( !mipped ) {
            // allocate mip map memory
            mipped = true;
        }
        if( !mip_utd ) {
            // calculate mip-maps
            mip_utd = true;
        }
    }
}

template< class T > void image< T >::de_mip() {  
    mipped = false;
    mip_utd = false;
    // deallocate all mip-maps
}

template< class T > void image< T >::reset() { 
    set_dim( { 0, 0 } );
    de_mip(); 
}

template< class T > void image< T >::use_mip( bool m ) {
    mip_me = m;
    if( mip_me ) mip_it();
    else {} // free mip-map memory?
}

template< class T > const vec2i image< T >::get_dim() const { return dim; }

// Reallocates base memory to match new dimensions, if needed
template< class T > void image< T >::set_dim( const vec2i& dims ) {
    if( dim != dims ) base.resize( dims.x * dims.y );
    dim = dims;
    refresh_bounds();
}

// calculates default bounding boxes based on pixel dimensions
template< class T > void image< T >::refresh_bounds() { 
    bounds.set( { -1.0, ( 1.0 * dim.y ) / dim.x }, { 1.0, ( -1.0 * dim.y ) / dim.x } );
    ipbounds.set( { 0, 0 }, dim );
    fpbounds.set( { 0.0f, 0.0f }, ( vec2f )dim - 1.0f );
}

template< class T > const bb2f image< T >::get_bounds() const { return bounds; }
template< class T > const bb2i image< T >::get_ipbounds() const { return ipbounds; }
template< class T > void image< T >::set_bounds( const bounding_box< float, 2 >& bb ) { bounds = bb; }

// returns true if images have same dimensions

template< class T > const T image< T >::index ( const vec2i& vi, const image_extend& extend ) const {

    T result;   // expect zero initialization

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

// interesting bug preserved in amber
template< class T > const T image< T >::sample_tile ( const vec2f& v, const bool& smooth, const image_extend& extend ) const {
    using namespace linalg;

    vec2i vi = ipbounds.bb_map( v, bounds );
    if( v.x < 0.0f ) vi.x -= 1;     // Correct for round towards zero
    if( v.y < 0.0f ) vi.y -= 1;
    if( smooth ) {  // linearly interpolated between neighboring values
        vec2f rem = v - bounds.bb_map( vi, ipbounds );  // Get remainders for interpolation
        return blend( blend ( index( vi ,               extend ), index( { vi.x + 1, vi.y     }, extend ), rem.x ),
                      blend ( index( { vi.x, vi.y + 1}, extend ), index( { vi.x + 1, vi.y + 1 }, extend ), rem.x ), rem.y );
 //       return lerp( lerp ( index( { vi.x + 1, vi.y + 1 }, extend ), index( { vi.x, vi.y + 1}, extend ), rem.x ), 
 //                    lerp ( index( { vi.x + 1, vi.y     }, extend ), index( vi ,               extend ), rem.x ), rem.y );
    }
    else return( index( vi , extend ) ); // quick and dirty sampling of nearest pixel value
}

template< class T > const T image< T >::sample ( const vec2f& v, const bool& smooth, const image_extend& extend ) const {
    using namespace linalg;
    vec2f vf = fpbounds.bb_map( v, bounds );
    vec2i vi = ( vec2i )vf;
    if( vf.x < 0.0f ) vi.x -= 1;     // Correct for round towards zero
    if( vf.y < 0.0f ) vi.y -= 1;
    if( smooth ) {  // linearly interpolated between neighboring values
        vec2f rem = vf - ( vec2f )vi;  // Get remainders for interpolation
        return blend( blend ( index( vi ,               extend ), index( { vi.x + 1, vi.y     }, extend ), rem.x ),
                      blend ( index( { vi.x, vi.y + 1}, extend ), index( { vi.x + 1, vi.y + 1 }, extend ), rem.x ), rem.y );
 //       return lerp( lerp ( index( { vi.x + 1, vi.y + 1 }, extend ), index( { vi.x, vi.y + 1}, extend ), rem.x ), 
 //                    lerp ( index( { vi.x + 1, vi.y     }, extend ), index( vi ,               extend ), rem.x ), rem.y );
    }
    else return( index( vi, extend ) ); // quick and dirty sampling of nearest pixel value
}

// Sets to background color everything outside of a centered circle
template< class T > void image< T >::crop_circle( const T& background, const float& ramp_width ) {
    float r2;   // radius in pixel space
    if( dim.x > dim.y ) r2 = dim.x / 2.0; 
    else r2 = dim.y / 2.0;
    float r1 = r2 * ( 1.0f - ramp_width );
    r1 = r1 * r1; r2 = r2 * r2;
    vec2f center = fpbounds.center();
    
    for( int x = 0; x < dim.x; x++ ) {
        for( int y = 0; y < dim.y; y++ ) {
            float r = linalg::length2( vec2f( { x * 1.0f, y * 1.0f } ) - center );
            if( r > r2 ) base[ y * dim.x + x ] = background;
            else blend( base[ y * dim.x + x ], background, ( 1.0f - sqrtf( r / r2 ) ) / ramp_width );
        }
    }
    mip_it(); 
}

// Colors black everything outside of a centered circle
template< class T > void image< T >::crop_circle( const float& ramp_width ) {
    T b; 
    black( b );
    crop_circle( b, ramp_width );
}

// copy image of same size ( may need to be able to scale as well )
template< class T > void image< T >::copy( const image< T >& img ) {
    set_dim( img.dim );
    set_bounds( img.bounds );
    std::copy( img.base.begin(), img.base.end(), base.begin() );
    mip_it();    
}

template< class T > void image< T >::fill( const T& c ) {
    std::fill( begin(), base.end(), c );
}

template< class T > void image< T >::fill( const T& c, const bb2i& bb ) {
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
    mip_it();
}

template< class T > void image< T >::fill( const T& c, const bb2f& bb ) {
    fill( c, bb.map_box( bounds, ipbounds ) );
}

template< class T > void image< T >::clear() {
    T b;
    black( b );
    fill( b );
}

// Black and white noise
template< class T > void image< T >::noise( const float& a ) {
    for( auto& pix : base ) { if( weighted_bit( a ) ) white( pix ); else black( pix ); }
}

template< class T > void image< T >::noise( const float& a, const bb2i& bb ) {
    if( ( bb.minv == ipbounds.minv ) && ( bb.maxv == ipbounds.maxv ) ) noise( a );
    else {
        auto bb1 = bb.intersect( ipbounds );
        for( int y = bb1.minv.y; y < bb1.maxv.y; y++ ) {
            for( int x = bb1.minv.x; x < bb1.maxv.x; x++ ) {
                if( weighted_bit( a ) ) white( base[ y * dim.x + x ] ); else black( base[ y * dim.x + x ] );
            }
        }
    }
    mip_it();
}

template< class T > void image< T >::noise( const float& a, const bb2f& bb ) {
    noise( a, bb.map_box( bounds, ipbounds ) );
}

template< class T > void image< T >::apply_mask( const image< T >& layer, const image< T >& mask, const mask_mode& mmode ) {
    auto l = layer.begin();
    auto m = mask.begin();
    for ( auto r = begin(); r <= end(); ) {
        ::apply_mask( *r, *l, *m, mmode );
        r++; l++; m++;
    }
    mip_it();
}

template< class T > void image< T >::splat( 
    const vec2f& center, 			    // coordinates of splat center
    const float& scale, 			    // radius of splat
    const float& theta, 			    // rotation in degrees
    const image< T >& splat_image,      // image of the splat
    const std::optional< std::reference_wrapper< image< T > > > mask,  // optional mask image
    const std::optional< T >&     tint, // change the color of splat
    const mask_mode& mmode              // how will mask be applied to splat and backround?
)  
{  
    const image< T >& g( splat_image );

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

    // *** Critical loop below ***
    // Quick and dirty sampling, high speed but risk of aliasing. 
    // Should work best if splat is fairly large and smooth.
    // future: add option to smooth sample into splat's mip-map
    // future: add vector and color effects
    if( has_mask ) {
        const image< T >& m = mask->get();
        // image and mask same size
        if( m.dim == g.dim ) {
            for( int x = sbounds.minv.x; x < sbounds.maxv.x; x++ ) {
                sfix = scfix;
                if( ( x >= 0 ) && ( x < dim.x ) ) {
                    for( int y = sbounds.minv.y; y < sbounds.maxv.y; y++ ) {
                        if( ( y >= 0 ) && ( y < dim.y ) && fixbounds.in_bounds( sfix ) ) {
                             if( has_tint ) {
                                ::apply_mask( base[ y * dim.x + x ], 
                                mulc( g.base[ (sfix.y >> 16) * g.dim.x + (sfix.x >> 16) ], my_tint ), 
                                m.base[ (sfix.y >> 16) * m.dim.x + (sfix.x >> 16) ], 
                                mmode ); }
                            else {
                                ::apply_mask( base[ y * dim.x + x ], 
                                g.base[ (sfix.y >> 16) * g.dim.x + (sfix.x >> 16) ], 
                                m.base[ (sfix.y >> 16) * m.dim.x + (sfix.x >> 16) ],
                                mmode );
                            }
                        }                        
                        sfix += unyfix;
                    }
                }
                scfix += unxfix;
            }
        }
        else {        
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
                            if( has_tint ) {
                                ::apply_mask( base[ y * dim.x + x ], 
                                mulc( g.base[ (sfix.y >> 16) * g.dim.x + (sfix.x >> 16) ], my_tint ), 
                                m.base[ (msfix.y >> 16) * m.dim.x + (msfix.x >> 16) ],
                                mmode ); }
                            else {
                                ::apply_mask( base[ y * dim.x + x ], 
                                g.base[ (sfix.y >> 16) * g.dim.x + (sfix.x >> 16) ], 
                                m.base[ (msfix.y >> 16) * m.dim.x + (msfix.x >> 16) ],
                                mmode );
                            }
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
                        if( has_tint ) addc( base[ y * dim.x + x ], mulc( g.base[ (sfix.y >> 16) * g.dim.x + (sfix.x >> 16) ], my_tint ) );
                        else           addc( base[ y * dim.x + x ], g.base[ (sfix.y >> 16) * g.dim.x + (sfix.x >> 16) ] );
                    }
                    sfix += unyfix;
                }
            }
            scfix += unxfix;
        }
    }
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
    mip_it();
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
    mip_it();
}

template< class T > void image< T >::warp ( const image< T >& in, 
                                            const image< int >& wf ) {
    if( !compare_dims( wf ) ) return; // Vector field and warp field must be same dimension
    if( !compare_dims( in ) ) return; // Vector field and input image must be same dimension
    std::transform( begin(), end(), wf.begin(), begin(), [ &in ] ( T &r, const unsigned int &i ) { return in.index( i ); } );
    mip_it();
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
    mip_it();
}

template< class T > void image< T >::read_binary(  const std::string &filename )
{
    vec2i new_dim;
    bb2f new_bounds;

    std::ifstream in_file( filename, std::ios::in | std::ios::binary );
    in_file.read( (char*)&new_dim, sizeof( vec2i ) );
    set_dim( new_dim );
    base.resize( dim.x * dim.y );

    in_file.read( (char*)&new_bounds, sizeof( bb2f ) );
    set_bounds( new_bounds );

    in_file.read( (char*)&(base[0]), dim.x * dim.y * sizeof( T ) );
    mip_it();
}

template< class T > void image< T >::write_binary( const std::string &filename )
{
    std::ofstream out_file( filename, std::ios::out | std::ios::binary );
    out_file.write( (char*)&dim, sizeof( vec2i ) );
    out_file.write( (char*)&bounds, sizeof( bb2f ) );
    out_file.write( (char*)&(base[0]), dim.x * dim.y * sizeof( T ) );
    out_file.close();
}

template< class T > void image< T >::write_file(const std::string &filename, file_type type, int quality ) {
    switch( type ) {
        case FILE_JPG: write_jpg( filename, quality ); break;
        case FILE_PNG: write_png( filename ); break;
        case FILE_BINARY: write_binary( filename ); break;
        default: std::cout << "image::write_file: unknown file type " << type << std::endl;
    }
}

// apply a vector function to each point in image
template< class T > void image< T >::apply( const std::function< T ( const T&, const float& ) > fn, const float& t ) {
    for( auto& v : base ) { v = fn( v, t ); }
    mip_it();
} 

// copy assignment
template< class T > image< T >& image< T >::operator = ( const image< T >& rhs ) {
    if( this != &rhs ) {
        base = rhs.base;
        dim = rhs.dim;
        bounds = rhs.bounds;
        ipbounds = rhs.ipbounds;
        mip_it();
    }
    return *this;
}

// move assignment
template< class T > image< T >& image< T >::operator = ( image< T >&& rhs ) {
    if( this != &rhs ) {
        base = std::move( rhs.base );
        dim = rhs.dim;
        bounds = rhs.bounds;
        ipbounds = rhs.ipbounds;
        mip_it();
    }
    return *this;
}

template< class T > image< T >& image< T >::operator += ( image< T >& rhs ) {
    using namespace linalg;
    std::transform( begin(), base.end(), rhs.begin(), begin(), [] ( const T &a, const T &b ) { return a + b; } );
    mip_it();
    return *this;
}

template< class T > image< T >& image< T >::operator += ( const T& rhs ) {
    using namespace linalg;
    std::transform( begin(), base.end(), begin(), [ rhs ]( const T &a ) { return a + rhs; } );
    mip_it();
    return *this;
}

template< class T > image< T >& image< T >::operator -= ( image< T >& rhs ) {
    using namespace linalg;
    std::transform( begin(), base.end(), rhs.begin(), begin(), []( const T &a, const T &b ) { return a - b; } );
    mip_it();
    return *this;
}

template< class T > image< T >& image< T >::operator -= ( const T& rhs ) {
    using namespace linalg;
    std::transform( begin(), base.end(), begin(), [ rhs ]( const T &a ) { return a - rhs; } );
    mip_it();
    return *this;
}

template< class T > image< T >& image< T >::operator *= ( image< T >& rhs ) {
    using namespace linalg;
    std::transform( begin(), base.end(), rhs.begin(), begin(), [] ( const T &a, const T &b ) { return a * b; } );
    mip_it();
    return *this;
}

template< class T > image< T >& image< T >::operator *= ( const T& rhs ) {
    using namespace linalg;
    std::transform( begin(), base.end(), begin(), [ rhs ]( const T &a ) { return a * rhs; } );
    mip_it();
    return *this;
}

template< class T > image< T >& image< T >::operator *= ( const float& rhs ) {
    using namespace linalg;
    std::transform( begin(), base.end(), begin(), [ rhs ]( const T &a ) { return ( T )( a * rhs ); } );
    mip_it();
    return *this;
}

template< class T > image< T >& image< T >::operator /= ( image< T >& rhs ) {
    using namespace linalg;
    std::transform( begin(), base.end(), rhs.begin(), begin(), [] ( const T &a, const T &b ) { return a / b; } );
    mip_it();
    return *this;
}

template< class T > image< T >& image< T >::operator /= ( const T& rhs ) {
    using namespace linalg;
    std::transform( begin(), base.end(), begin(), [ rhs ]( const T &a ) { return a / rhs; } );
    mip_it();
    return *this;
}

template< class T > image< T >& image< T >::operator /= ( const float& rhs ) {
    using namespace linalg;
    std::transform( begin(), base.end(), begin(), [ rhs ]( const T &a ) { return ( T )( a / rhs ); } );
    mip_it();
    return *this;
}

template< class T > image< T >& image< T >::operator () () { return *this; }

template class image< frgb   >;      // fimage
template class image< ucolor >;      // uimage
template class image< vec2f  >;      // vector_field
//template class image< float >;     // scalar_field
template class image< int    >;      // warp_field
template class image< vec2i  >;      // offset_field 
