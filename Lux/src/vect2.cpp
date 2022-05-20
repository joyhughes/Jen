#include <iostream>
#include "vect2.hpp"

#define R x
#define THETA y

using namespace linalg;

float vtoa( const vec2f& v )
{
	float ang;

	ang = atan2f( v.y, v.x ) / TAU * 360.0f;
	if( ang < 0.0 ) ang += 360.0f;
	return ang;
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
vec2f complement( const vec2f& v )     { return { -v.y, v.x }; }

vec2f radial( const vec2f& v )         { return {linalg::length( v ), vtoa( v ) }; }

vec2f cartesian( const vec2f& rad )    { return { rad.R * cos_deg( rad.THETA ), rad.R * sin_deg( rad.THETA ) }; }

vec2f inverse_square( const vec2f& in, const float& diameter, const float& soften )
{
	vec2f out;
	float mag;

	if( ( in.x == 0.0) && ( in.y == 0.0 ) ) {
		return { 0.0f, 0.0f };
	}
	else {
		mag = diameter * diameter * sqrtf( 1.0 / ( length2( in ) + soften * soften ) );
		out = normalize( in );
		out.x *= mag; out.y *= mag;
		return out;
	}
}

vec2f complex_power( const vec2f& in, const float& p )
{
	vec2f out; 

	out = radial( in );
	out.R = powf( out.R, p );
	out.THETA *= p;
	out = cartesian( out );
	return in;
} 

// testing purposes only
int main() {
    using std::cout;
    using namespace ostream_overloads;

    vec2f a = { 1.0f, 2.0f };
    cout << "vector a " << a << "\n";

    a = normalize( a );
    cout << "normalized " << a << "\n";

    auto b = rot( 1.0f, a );
    cout << "rotated with rot() " << b << "\n";

    mat2f rm = rotation_matrix_2D< float >( 1.0f );

    auto c = mul( rm, a );
    cout << "rotated with matrix " << c << "\n";

    auto d = less( vec2f( { 1.0f, 0.0f } ), vec2f( { 0.0f, 1.0f } ) );
    cout << "less test " << d << "\n";

    bounding_box< float, 2 > bb( vec2f( -1.0f, -1.0f ), vec2f( 1.0f, 1.0f ) );
    vec2f e(  0.5f, 0.5f );
    vec2f f( -1.5f, 1.5f );
    cout << "{ 0.5, 0.5 }  in bounds        " << bb.in_bounds( e )     << "\n";
    cout << "{ -1.5, 1.5 } in bounds        " << bb.in_bounds( f )     << "\n";
    bb.pad( 1.0f );
    cout << "{ 0.5, 0.5 }  in padded bounds " << bb.in_bounds_pad( e ) << "\n";
    cout << "{ -1.5, 1.5 } in padded bounds " << bb.in_bounds_pad( f ) << "\n";

    cout << "\nbox_of_random() test\n";
    for( int i = 0; i < 10; i++ ) { cout << bb.box_of_random() << " "; }
    cout << "\n";

    return 0;
}


