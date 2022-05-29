// Jen uses vectors based on linalg.h
// Includes vector functions used in Jen that are not included in linalg.h and a bounding box class
// Jen generates angles in degrees for readability - degree based trigonometry functions included

#ifndef __VECT2_HPP
#define __VECT2_HPP

#ifndef R   
    #define R x
#endif // R

#ifndef THETA
    #define THETA y
#endif // THETA

#define TAU 6.283185307179586f

#include "linalg.h"

typedef linalg::vec< float,2 > vec2f;
typedef linalg::vec< int  ,2 > vec2i;
typedef linalg::mat< float, 2, 2 > mat2f;

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
    V b1, b2;  // Bounds define opposite corners of the box. Inputs to constructor do not need to be minimum and maximum - this is calculated automatically.
    V minv, maxv;  // Per-component vectors sorted so mins[ foo ] <= maxs[ foo ] is always true
    V minp, maxp;  // Padded bounding box values to calculate if point is within a specified distance to bounding box in any dimension

    bounding_box()                           : 
        b1( { -1.0f, -1.0f } ), b2( { 1.0f, 1.0f } ), minv( b1 ), maxv( b2 ), minp( b1 ), maxp( b2 )       { }    
    bounding_box( const V& v1, const V& v2 ) : 
        b1( v1 ), b2(v2), minv( min( v1, v2 ) ), maxv( max( v1, v2 ) ), minp( minv ), maxp( maxv ) { }
    // copy+conversion constructor
    template< class U > bounding_box( const bounding_box< U, M >& bbc ) :
        b1( bbc.b1 ), b2( bbc.b2 ), minv( min( b1, b2 ) ), maxv( max( b1, b2 ) ), minp( minv ), maxp( maxv ) { }

    void set( const V& v1, const V& v2 ) { b1 = v1; b2 = v2; minv = min( v1, v2 ); maxv = max( v1, v2 ); minp = minv; maxp = maxv; }

    bool in_bounds(               const V& v ) { return linalg::all( linalg::gequal(  v, minv ) & linalg::lequal( v, maxv ) ); }
    bool in_bounds_pad(           const V& v ) { return linalg::all( linalg::gequal(  v, minp ) & linalg::lequal( v, maxp ) ); }
    bool in_bounds_exclusive(     const V& v ) { return linalg::all( linalg::greater( v, minv ) & linalg::less(   v, maxv ) ); }
    bool in_bounds_exclusive_pad( const V& v ) { return linalg::all( linalg::greater( v, minp ) & linalg::less(   v, maxp ) ); }
    bool in_bounds_half_open(     const V& v ) { return linalg::all( linalg::gequal(  v, minv ) & linalg::less(   v, maxv ) ); }
    bool in_bounds_half_open_pad( const V& v ) { return linalg::all( linalg::gequal(  v, minp ) & linalg::less(   v, maxp ) ); }

    void pad( const T& p ) { minp = minv - p; maxp = maxv + p; }      // pads the bounding box by a fixed amount in all dimensions
    void pad( const V& p ) { minp = minv - p; maxp = maxv + p; }      // pads the bounding box by a specified amount in each dimension, represented by a vector

    // returns a random point within the box using a uniform distribution
    V box_of_random() { return linalg::rbox( minv, maxv ); } 

    // Linear map from one bounding box to another
    template< class U > V bb_map( const linalg::vec< U, M >& in, const bounding_box< U, M >& target )
    { return linalg::cmul( ( in - target.b1 ), ( b2 - b1 ) ) / ( target.b2 - target.b1 ) + b1; }
};

typedef bounding_box< int, 2 > bb2i;
typedef bounding_box< float, 2 > bb2f;
typedef bounding_box< float, 3 > bb3f;

#endif // __VECT2_HPP