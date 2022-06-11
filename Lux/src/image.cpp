// Template base class for rasterized data - used as base for image classes and vector fields
// Handles functions common to all of these classes.
// Supports linear interpolated sampling and mip-mapping - multiple resolutions for anti-aliasing

#include "image.hpp"

#define I image<T>

template< class T > void I :: mip_it() { // mip it good
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

template< class T > void I :: de_mip() {  
    mipped = false;
    mip_utd = false;
    // deallocate all mip-maps
}

template< class T > void I :: reset() { 
    base.clear();
    set_dim( { 0, 0 } );
    de_mip(); 
}

template< class T > void I :: use_mip( bool m ) {
    mip_me = m;
    if( mip_me ) mip_it();
    else {} // free mip-map memory?
}

template< class T > vec2i I :: get_dim() { return dim; }

template< class T > void I :: set_dim( const vec2i& dims ) {
    dim = dims;
    ipbounds.set( { 0, 0 }, dim );
}

template< class T > bb2f I :: get_bounds() { return bounds; }
template< class T > void I :: set_bounds( const bounding_box< float, 2 >& bb ) { bounds = bb; }

// returns true if images have same dimensions
// template< class T, class U > bool I :: compare_dims( const image< U >& img ) { return ( dim == img.dim ); } 

template< class T > void I :: fill( const T& c ) {
    std :: fill( base.begin(), base.end(), c );
    mip_it();
}

template< class T > I& I :: operator += ( I& rhs ) {
    using namespace linalg;
    std :: transform( base.begin(), base.end(), rhs.base.begin(), base.begin(), [] ( const T &a, const T &b ) { return a + b; } );
    mip_it();
    return *this;
}

template< class T > I& I :: operator += ( const T& rhs ) {
    using namespace linalg;
    std :: transform( base.begin(), base.end(), base.begin(), [ rhs ]( const T &a ) { return a + rhs; } );
    mip_it();
    return *this;
}

template< class T > I& I :: operator -= ( I& rhs ) {
    using namespace linalg;
    std :: transform( base.begin(), base.end(), rhs.base.begin(), base.begin(), []( const T &a, const T &b ) { return a - b; } );
    mip_it();
    return *this;
}

template< class T > I& I :: operator -= ( const T& rhs ) {
    using namespace linalg;
    std :: transform( base.begin(), base.end(), base.begin(), [ rhs ]( const T &a ) { return a - rhs; } );
    mip_it();
    return *this;
}

template< class T > I& I :: operator *= ( I& rhs ) {
    using namespace linalg;
    std :: transform( base.begin(), base.end(), rhs.base.begin(), base.begin(), [] ( const T &a, const T &b ) { return a * b; } );
    mip_it();
    return *this;
}

template< class T > I& I :: operator *= ( const T& rhs ) {
    using namespace linalg;
    std :: transform( base.begin(), base.end(), base.begin(), [ rhs ]( const T &a ) { return a * rhs; } );
    mip_it();
    return *this;
}

template< class T > I& I :: operator *= ( const float& rhs ) {
    using namespace linalg;
    std :: transform( base.begin(), base.end(), base.begin(), [ rhs ]( const T &a ) { return a * rhs; } );
    mip_it();
    return *this;
}
template< class T > I& I :: operator /= ( I& rhs ) {
    using namespace linalg;
    std :: transform( base.begin(), base.end(), rhs.base.begin(), base.begin(), [] ( const T &a, const T &b ) { return a / b; } );
    mip_it();
    return *this;
}

template< class T > I& I :: operator /= ( const T& rhs ) {
    using namespace linalg;
    std :: transform( base.begin(), base.end(), base.begin(), [ rhs ]( const T &a ) { return a / rhs; } );
    mip_it();
    return *this;
}

template< class T > I& I :: operator /= ( const float& rhs ) {
    using namespace linalg;
    std :: transform( base.begin(), base.end(), base.begin(), [ rhs ]( const T &a ) { return a / rhs; } );
    mip_it();
    return *this;
}

template class image< frgb >;       // fimage
template class image< ucolor >;     // uimage
template class image< vec2f >;      // vector_field
