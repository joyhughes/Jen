// Template base class for rasterized data - used as base for image classes and vector fields
// Handles functions common to all of these classes.
// Supports linear interpolated sampling and mip-mapping - multiple resolutions for anti-aliasing

#include <optional>
#include "image.hpp"
#include "vector_field.hpp"

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
    base.clear();
    set_dim( { 0, 0 } );
    de_mip(); 
}

template< class T > void image< T >::use_mip( bool m ) {
    mip_me = m;
    if( mip_me ) mip_it();
    else {} // free mip-map memory?
}

template< class T > const vec2i image< T >::get_dim() const { return dim; }

template< class T > void image< T >::set_dim( const vec2i& dims ) {
    dim = dims;
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
        // if( !ipbounds.in_bounds_half_open( { x, y } ) ) std::cout << "vi: " << vi.x << " " << vi.y << " xblock: " << xblock << " yblock: " << yblock << " { x, y }:" << x << " " << y << "\n";
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
        std::cout << "sample v = " << v.x << " " << v.y << " vi = " << vi.x << " " << vi.y << " rem " << rem.x << " " << rem.y << "\n";
        return lerp( lerp ( index( vi ,               extend ), index( { vi.x + 1, vi.y     }, extend ), rem.x ),
                     lerp ( index( { vi.x, vi.y + 1}, extend ), index( { vi.x + 1, vi.y + 1 }, extend ), rem.x ), rem.y );
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
        //std::cout << "sample v = " << v.x << " " << v.y << " vi = " << vi.x << " " << vi.y << " rem " << rem.x << " " << rem.y << "\n";
        return lerp( lerp ( index( vi ,               extend ), index( { vi.x + 1, vi.y     }, extend ), rem.x ),
                     lerp ( index( { vi.x, vi.y + 1}, extend ), index( { vi.x + 1, vi.y + 1 }, extend ), rem.x ), rem.y );
 //       return lerp( lerp ( index( { vi.x + 1, vi.y + 1 }, extend ), index( { vi.x, vi.y + 1}, extend ), rem.x ), 
 //                    lerp ( index( { vi.x + 1, vi.y     }, extend ), index( vi ,               extend ), rem.x ), rem.y );
    }
    else return( index( vi, extend ) ); // quick and dirty sampling of nearest pixel value
}

template< class T > void image< T >::fill( const T& c ) {
    std::fill( begin(), base.end(), c );
    mip_it();
}

template< class T > void image< T >::apply_mask( const image< T >& layer, const image< T >& mask ) {
    auto l = layer.begin();
    auto m = mask.begin();
    for ( auto r = begin(); r <= end(); ) {
        ::apply_mask( *r, *l, *m );
        r++; l++; m++;
    }
    mip_it();
}

template< class T > void image< T >::splat( 
        const vec2f& center, 			// coordinates of splat center
        const float& scale, 			// radius of splat
        const float& theta, 			// rotation in degrees
        std::optional< T >& tint,		// change the color of splat
        const image< T >& g 	)	                // image of the splat
{
    float thrad = theta / 360.0 * TAU;             // theta in radians
    vec2i p = ipbounds.bb_map( center, bounds);     // center of splat in pixel coordinates
	int size = scale / ( bounds.b2.x - bounds.b1.x ) * dim.x; // scale in pixel coordinates
    bb2i sbounds( p, size );    // bounding box of splat
	vec2f smin = bounds.bb_map( sbounds.minv, ipbounds );
	vec2f sc;
	sc.x = (smin.x - center.x) / scale;
	sc.y = -(smin.y - center.y) / scale;
    sc = linalg::rot( -thrad, sc );

	// calculate unit vectors - one pixel long
	vec2f unx, uny;
	unx.x = 1.0f / dim.x * ( bounds.b2.x - bounds.b1.x ) / scale;
	unx.y = 0.0f;
	unx = linalg::rot( -thrad, unx );
	uny.x = 0.0f;
	uny.y = 1.0f / dim.y * ( bounds.b2.y - bounds.b1.y ) / scale;
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
    // future: add mask
    // future: add effects ( warp, melt, hyper, life )

	for( int x = sbounds.minv.x; x < sbounds.maxv.x; x++ ) {
        sfix = scfix;
		if( ( x >= 0 ) && ( x < dim.x ) ) {
			for( int y = sbounds.minv.y; y < sbounds.maxv.y; y++ ) {
				if( ( y >= 0 ) && ( y < dim.y ) && fixbounds.in_bounds( sfix ) ) {
                    if( tint.has_value() ) base[ y * dim.x + x ] += linalg::cmul( g.base[ (sfix.y >> 16) * g.dim.x + (sfix.x >> 16) ], *tint );
                    else          base[ y * dim.x + x ] +=               g.base[ (sfix.y >> 16) * g.dim.x + (sfix.x >> 16) ];
				}
				sfix += unyfix;
			}
		}
		scfix += unxfix;
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
                coord = bounds.bb_map( { x, y }, ipbounds );
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
            vec2f coord = bounds.bb_map( { x, y }, ipbounds );
            vec2f v = vfn( coord );
            if( relative ) v = v * step + coord;
            *it = in.sample( v, smooth, extend );
            it++;
        }
    }
    mip_it();
}

// apply a vector function to each point in image
template< class T > void image< T >::apply( const std::function< T ( const T&, const float& ) > fn, const float& t ) {
    for( auto& v : base ) { v = fn( v, t ); }
    mip_it();
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
    std::transform( begin(), base.end(), begin(), [ rhs ]( const T &a ) { return a * rhs; } );
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
    std::transform( begin(), base.end(), begin(), [ rhs ]( const T &a ) { return a / rhs; } );
    mip_it();
    return *this;
}

template< class T > image< T >& image< T >::operator () () { return *this; }

template class image< frgb >;       // fimage
template class image< ucolor >;     // uimage
template class image< vec2f >;      // vector_field
