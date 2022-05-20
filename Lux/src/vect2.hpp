// Vector functions used in Jen that are not included in linalg.h
// Some definitions included in header to ease potential fork of linalg.h

#ifndef __VECT2_HPP
#define __VECT2_HPP

#define TAU 6.283185307179586f

#include "linalg.h"

typedef linalg::vec< float,2 > vec2f;
typedef linalg::mat< float, 2, 2 > mat2f;

// necessary if using C++11 random functions?
// float rand1();

// degree-based trig functions
// Jen generates angles in degrees for readability 
float sin_deg( const float& theta );
float cos_deg( const float& theta );
float tan_deg( const float& theta );
float vtoa( const vec2f& v );
float add_angle( const float& a1, const float& a2 );

// Vector functions used in Jen not included in linalg.h
// future: add complex multiply
vec2f complement( const vec2f& v );
vec2f v_radial( const vec2f& v );
vec2f v_cartesian( const vec2f& rad );
vec2f inverse_square( const vec2f& in, const float& diameter, const float& soften );
vec2f complex_power( const vec2f& in, const float& p );

// Axis-aligned bounding box
template< class T, int M > struct bounding_box {
    typedef linalg::vec< T, M >            V;
    V bound1, bound2;  // Bounds define opposite corners of the box. Inputs to constructor do not need to be minimum and maximum - this is calculated automatically.
    V minv, maxv;  // Per-component vectors sorted so mins[ foo ] <= maxs[ foo ] is always true
    V minp, maxp;  // Padded bounding box values to calculate if point is within a specified distance to bounding box in any dimension

    bounding_box()                           : bound1(), bound2(), minv(), maxv(), minp(), maxp()       { }    
    bounding_box( const V& v1, const V& v2 ) : 
        bound1( v1 ), bound2(v2), minv( min( v1, v2 ) ), maxv( max( v1, v2 ) ), minp( minv ), maxp( maxv ) { }

    void set( V v1, V v2 ) { bound1 = v1; bound2 = v2; minv = min( v1, v2 ); maxv = max( v1, v2 ); minp = minv; maxp = maxv; }

    bool in_bounds(     const V& v )           { return linalg::all( linalg::gequal( v, minv )  & linalg::lequal( v, maxv ) ); }
    bool in_bounds_pad( const V& v )           { return linalg::all( linalg::gequal( v, minp )  & linalg::lequal( v, maxp ) ); }
    bool in_bounds_exclusive(     const V& v ) { return linalg::all( linalg::greater( v, minv ) & linalg::less( v, maxv ) ); }
    bool in_bounds_exclusive_pad( const V& v ) { return linalg::all( linalg::greater( v, minp ) & linalg::less( v, maxp ) ); }

    void pad( const T& p ) { minp = minv - p; maxp = maxv + p; }      // pads the bounding box by a fixed amount in all dimensions
    void pad( const V& p ) { minp = minv - p; maxp = maxv + p; }      // pads the bounding box by a specified amount in each dimension, represented by a vector

    // returns a random point within the box using a uniform distribution
    V box_of_random() { return linalg::rbox( minv, maxv ); }
};

#endif // __VECT2_HPP