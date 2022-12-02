// Jen uses vectors based on linalg.h
// Includes vector functions used in Jen that are not included in linalg.h and a bounding box class
// Jen generates angles in degrees for readability - degree based trigonometry functions included

#include "vect2.hpp"

using namespace linalg;

float vtoa( const vec2f& v )
{
	float ang;

	ang = atan2f( v.y, v.x ) / TAU * 360.0f;
	if( ang < 0.0 ) ang += 360.0f;
	return ang;
}

vec2f unit_vector( const float& theta )
{
	return { cos_deg( theta ), sin_deg( theta ) };
}

float add_angle( const float& a1, const float& a2 )
{
	float a = fmodf(a1 + a2, 360.0 );
	if( a < 0.0 ) a += 360.0;
	return a;
}

float sin_deg( const float& theta )
{
	return sinf( theta / 360.0 * TAU );
}

float cos_deg( const float& theta )
{
	return cosf( theta / 360.0 * TAU );
}

float tan_deg( const float& theta )
{
	return tanf( theta / 360.0 * TAU );
}

// returns a vector 90 degrees to the left of input vector
// equivalent to complex multiply by i
vec2f rot_deg( const vec2f& v, const float& ang_deg ) { return linalg::rot( ang_deg / 360.0f * TAU, v ); }

vec2f complement( const vec2f& v )     { return { -v.y, v.x }; }

vec2f radial( const vec2f& v )         { return {linalg::length( v ), vtoa( v ) }; }

vec2f cartesian( const vec2f& rad )    { return { rad.R * cos_deg( rad.THETA ), rad.R * sin_deg( rad.THETA ) }; }

vec2f inverse( const vec2f& v, const float& diameter, const float& soften ) {
	if( ( ( v.x == 0.0f ) && ( v.y == 0.0f ) && ( soften == 0.0f ) ) || ( diameter == 0.0f ) ) return { 0.0f, 0.0f };
	return v / ( linalg::length2( v ) / diameter + soften );
}

// less efficient - deprecated
vec2f inverse_square( const vec2f& in, const float& diameter, const float& soften )
{
	vec2f out;
	float mag;

	if( ( in.x == 0.0) && ( in.y == 0.0 ) && ( soften == 0.0f ) ) {
		return { 0.0f, 0.0f };
	}
	else {
		mag = diameter * diameter * sqrtf( 1.0 / ( length2( in ) + soften * soften ) );
		out = normalize( in );
		out.x *= mag; out.y *= mag;
		return out;
	}
}

vec2f complex_power( const vec2f& in, const float& p ) {
	vec2f out; 

	out = radial( in );
	out.R = powf( out.R, p );
	out.THETA *= p;
	out = cartesian( out );
	return out;
} 

void apply_mask( vec2f& result, const vec2f& layer, const vec2f& mask, const mask_mode& mmode ) {
    if( mmode %2 )  {   // mask affects splat
        if( mmode % 1 ) result = linalg::lerp( result, layer, mask );
        else result = linalg::cmul( result, ( vec2f( { 1.0f, 1.0f } ) - mask ) )  + layer;
    }
    else{
        if( mmode % 1 ) result += linalg::cmul( layer, mask );
        else result += layer;   // mask has no effect
    }    
}






