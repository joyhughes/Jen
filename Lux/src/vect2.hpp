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
#include "mask_mode.hpp"
#include <iostream>

typedef linalg::vec< float, 2 > vec2f;
typedef linalg::vec< int,   2 > vec2i;
typedef linalg::mat< float, 2, 2 > mat2f;

// Time-dependent vector function
typedef std::function< vec2f ( const vec2f&, const float& ) > vector_fn;    

enum rotation_direction
{
	COUNTERCLOCKWISE,
	CLOCKWISE,
	RANDOM,
    LAVA_LAMP
};
typedef enum rotation_direction rotation_direction;

// degree-based trig functions
// Jen generates angles in degrees for readability 
float sin_deg( const float& theta );
float cos_deg( const float& theta );
float tan_deg( const float& theta );
float vtoa( const vec2f& v );
vec2f unit_vector( const float& theta );
float add_angle( const float& a1, const float& a2 );

// Vector functions used in Jen not included in linalg.h
// future: add complex multiply
vec2f rot_deg( const vec2f& v, const float& ang_deg );
vec2f complement( const vec2f& v );
vec2f radial( const vec2f& v );
vec2f cartesian( const vec2f& rad );
vec2f inverse_square( const vec2f& in, const float& diameter, const float& soften = 0.0f ); // less efficient
vec2f inverse( const vec2f& in, const float& diameter, const float& soften = 0.0f ); // less efficient
vec2f complex_power( const vec2f& in, const float& p );

// masking functions
void apply_mask( vec2f& result, const vec2f& layer, const vec2f& mask, const mask_mode& mmode = MASK_BLEND  );

// included for cellular automata operating on vector fields (weird but maybe cool?)
inline void white( vec2f& w ) { w = vec2f( { 1.0f, 1.0f } ); }
inline void black( vec2f& b ) { b = vec2f( { 0.0f, 0.0f } ); }

// Axis-aligned bounding box
template< class T, int M > struct bounding_box {
    typedef linalg::vec< T, M > V;
    V b1, b2;  // Bounds define opposite corners of the box. Inputs to constructor do not need to be minimum and maximum - this is calculated automatically.
    V minv, maxv;  // Per-component vectors sorted so mins[ foo ] <= maxs[ foo ] is always true
    V minp, maxp;  // Padded bounding box values to calculate if point is within a specified distance to bounding box in any dimension

    bounding_box()                           : 
        b1( { -1.0f, -1.0f } ), b2( { 1.0f, 1.0f } ), minv( linalg::min( b1, b2 ) ), maxv( linalg::max( b1, b2 ) ), minp( minv ), maxp( maxv ) { }  

    bounding_box( const V& v1, const V& v2 ) : 
        b1( v1 ), b2( v2 ), minv( linalg::min( v1, v2 ) ), maxv( linalg::max( v1, v2 ) ), minp( minv ), maxp( maxv ) { }

    bounding_box( const V& v, const T& size ) :
        b1( v - size ), b2( v + size ), minv( linalg::min( b1, b2 ) ), maxv( linalg::max( b1, b2 ) ), minp( minv ), maxp( maxv ) { }
           
    // copy+conversion constructor
    template< class U > bounding_box( const bounding_box< U, M >& bbc ) :
        b1( bbc.b1 ), b2( bbc.b2 ), minv( linalg::min( b1, b2 ) ), maxv( linalg::max( b1, b2 ) ), minp( minv ), maxp( maxv ) { }

    void set( const V& v1, const V& v2 )
    { b1 = v1; b2 = v2; minv = linalg::min( v1, v2 ); maxv = linalg::max( v1, v2 ); minp = minv; maxp = maxv; }

    bool in_bounds(               const V& v ) const
    { return linalg::all( linalg::gequal(  v, minv ) & linalg::lequal( v, maxv ) ); }

    bool in_bounds_pad(           const V& v ) const
    { return linalg::all( linalg::gequal(  v, minp ) & linalg::lequal( v, maxp ) ); }

    bool in_bounds_exclusive(     const V& v ) const
    { return linalg::all( linalg::greater( v, minv ) & linalg::less(   v, maxv ) ); }

    bool in_bounds_exclusive_pad( const V& v ) const
    { return linalg::all( linalg::greater( v, minp ) & linalg::less(   v, maxp ) ); }

    bool in_bounds_half_open(     const V& v ) const
    { return linalg::all( linalg::gequal(  v, minv ) & linalg::less(   v, maxv ) ); }

    bool in_bounds_half_open_pad( const V& v ) const
    { return linalg::all( linalg::gequal(  v, minp ) & linalg::less(   v, maxp ) ); }

    void pad( const T& p )  // pads the bounding box by a fixed amount in all dimensions
    { minp = minv - p; maxp = maxv + p; }

    void pad( const V& p )  // pads the bounding box by a specified amount in each dimension, represented by a vector
    { minp = minv - p; maxp = maxv + p; }

    // return a random point within the box using a uniform distribution
    V box_of_random() const { return linalg::rbox( minv, maxv ); }

    // Linear map from one bounding box to another
    template< class U > V bb_map( const linalg::vec< U, M >& in, const bounding_box< U, M >& target ) const
    { return ( V )linalg::cmul( ( vec2f )( in - target.minv ), ( vec2f )( maxv - minv ) ) / ( V )( target.maxv - target.minv ) + minv; }

    // map bounding box from linear space of one box to another
    template< class U > bounding_box< T, M > map_box( const bounding_box< U, M >& in, const bounding_box< U, M >& target ) const
    { return bounding_box< T, M > ( bb_map( in.b1, target ), bb_map( in.b2, target ) ); }

    // return intersection of two bounding boxes
    template< class U > bounding_box< T, M > intersect( const bounding_box< U, M >& in ) const
    { return bounding_box< U, M > ( max( minv, in.minv ), min( maxv, in.maxv ) ); }

    V center() { return ( b1 + b2 ) / 2.0f; }

    void print() { std::cout << "[ [ " << b1.x << ", " << b1.y << " ], [ " << b2.x << ", " << b2.y << " ] ]" << std::endl; }
};

typedef bounding_box< int,   2 > bb2i;
typedef bounding_box< float, 2 > bb2f;
typedef bounding_box< float, 3 > bb3f;

#endif // __VECT2_HPP