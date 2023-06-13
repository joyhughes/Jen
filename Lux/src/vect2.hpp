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
// vector_field
void apply_mask( vec2f& result, const vec2f& layer, const vec2f& mask, const mask_mode& mmode = MASK_BLEND  );
static inline vec2f blend( const vec2f& a, const vec2f& b, const float& prop ) { return linalg::lerp( a, b, prop ); }

// warp_field
void apply_mask( int& result, const int& layer, const int& mask, const mask_mode& mmode = MASK_BLEND  );
static inline int blend( const int& a, const int& b, const float& prop ) { return (int) std::lerp( (float) a, (float) b, prop ); }

// offset_field
void apply_mask( vec2i& result, const vec2i& layer, const vec2i& mask, const mask_mode& mmode = MASK_BLEND  );
static inline vec2i blend( const vec2i& a, const vec2i& b, const float& prop ) { return vec2i( { (int) std::lerp( (float) a.x, (float) b.x, prop ), (int) std::lerp( (float) a.y, (float) b.y, prop ) } ); }

// default colors for various image operations
// imagine cellular automata operating on vector fields (weird but maybe cool?)
// vector_field
inline void white( vec2f& w ) { w = vec2f( { 1.0f, 1.0f } ); }
inline void black( vec2f& b ) { b = vec2f( { 0.0f, 0.0f } ); }

// warp_field
inline void white( int& w ) { w = 1; }
inline void black( int& b ) { b = 0; }

// offset_field
inline void white( vec2i& w ) { w = vec2i( { 1, 1 } ); }
inline void black( vec2i& b ) { b = vec2i( { 0, 0 } ); }

// scalar field
inline void white( float& w ) { w = 1.0f; }
inline void black( float& b ) { b = 0.0f; }

// wrappers for addition and subtraction - used for image compatibility
// vector_field
static inline void  addc( vec2f& c1, const vec2f& c2 ) { c1 += c2; }
static inline void  subc( vec2f& c1, const vec2f& c2 ) { c1 -= c2; }
static inline vec2f mulc( const vec2f& c1, const vec2f& c2 ) { return linalg::cmul( c1, c2 ); }

// warp_field
static inline void  addc( int& c1, const int& c2 ) { c1 += c2; }
static inline void  subc( int& c1, const int& c2 ) { c1 -= c2; }
static inline int   mulc( const int& c1, const int& c2 ) { return c1 * c2; }

// offset_field
static inline void  addc( vec2i& c1, const vec2i& c2 ) { c1 += c2; }
static inline void  subc( vec2i& c1, const vec2i& c2 ) { c1 -= c2; }
static inline vec2i mulc( const vec2i& c1, const vec2i& c2 ) { return linalg::cmul( c1, c2 ); }

// scalar field
static inline void  addc( float& c1, const float& c2 ) { c1 += c2; }
static inline void  subc( float& c1, const float& c2 ) { c1 -= c2; }
static inline float mulc( const float& c1, const float& c2 ) { return c1 * c2; }

// template function that returns a value of all ones for a given type
template< class T > T iden() {}
template<> inline vec2f iden< vec2f >() { return vec2f( { 1.0f, 1.0f } ); }
template<> inline vec2i iden< vec2i >() { return vec2i( { 1 , 1 } ); }
template<> inline float iden< float >() { return 1.0f; }
template<> inline int   iden< int >  () { return 1; }

// Axis-aligned bounding box
template< class T, int M > struct bounding_box {
    typedef linalg::vec< T, M > V;
    V b1, b2;  // Bounds define opposite corners of the box. Inputs to constructor do not need to be minimum and maximum - this is calculated automatically.
    V minv, maxv;  // Per-component vectors sorted so mins[ foo ] <= maxs[ foo ] is always true
    V minp, maxp;  // Padded bounding box values to calculate if point is within a specified distance to bounding box in any dimension

    bounding_box()                           : 
        b1( { -1.0f, -1.0f } ), b2( { 1.0f, 1.0f } ), minv( linalg::min( b1, b2 ) ), maxv( linalg::max( b1, b2 ) ), minp( minv ), maxp( maxv ) { }  

    bounding_box( const V& v )               :
        b1( 0 ), b2( v ), minv( linalg::min( 0, v ) ), maxv( linalg::max( 0, v ) ), minp( minv ), maxp( maxv ) { }

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
    template< class U, class W > V bb_map( const linalg::vec< U, M >& in, const bounding_box< W, M >& target ) const
    { return ( V )linalg::cmul( ( vec2f )( in - target.minv ), ( vec2f )( maxv - minv ) ) / ( V )( target.maxv - target.minv ) + minv; }

    // map bounding box from linear space of one box to another
    template< class U, class W > bounding_box< T, M > map_box( const bounding_box< U, M >& in, const bounding_box< W, M >& target ) const
    { return bounding_box< T, M > ( bb_map( in.b1, target ), bb_map( in.b2, target ) ); }

    // return intersection of two bounding boxes
    template< class U > bounding_box< T, M > intersect( const bounding_box< U, M >& in ) const
    { return bounding_box< U, M > ( max( minv, in.minv ), min( maxv, in.maxv ) ); }

    V center() { return ( b1 + b2 ) / 2.0f; }

    void print() const { std::cout << "[ [ " << b1.x << ", " << b1.y << " ], [ " << b2.x << ", " << b2.y << " ] ]" << std::endl; }
};

typedef bounding_box< int,   2 > bb2i;
typedef bounding_box< float, 2 > bb2f;
typedef bounding_box< float, 3 > bb3f;

static inline void addc( bb2i& b, const bb2i& v )
{
    b.b1.x += v.b1.x;
    b.b1.y += v.b1.y;
    b.b2.x += v.b2.x;
    b.b2.y += v.b2.y;
}

static inline void addc( bb2f& b, const bb2f& v )
{
    b.b1.x += v.b1.x;
    b.b1.y += v.b1.y;
    b.b2.x += v.b2.x;
    b.b2.y += v.b2.y;
}

static inline void addc( bb3f& b, const bb3f& v )
{
    b.b1.x += v.b1.x;
    b.b1.y += v.b1.y;
    b.b1.z += v.b1.z;
    b.b2.x += v.b2.x;
    b.b2.y += v.b2.y;
    b.b2.z += v.b2.z;
}

#endif // __VECT2_HPP
