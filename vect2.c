#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "vect2.h"


// returns a random number between 0 and 1
float rand1()
{
	return (rand() * 1.0) / (RAND_MAX * 1.0);
}

float vtoa( vect2 v )
{
	float ang;

	ang = atan2( v.y, v.x ) / TAU * 360.0;
	if( ang < 0.0 ) ang += 360.0;
	return ang;
}

float add_angle( float a1, float a2 )
{
	float a = fmod(a1 + a2, 360.0 );
	if( a < 0.0 ) a += 360.0;
	return a;
}

float sin_deg( float theta )
{
	return sin( theta / 360.0 * TAU );
}

float cos_deg( float theta )
{
	return cos( theta / 360.0 * TAU );
}

float tan_deg( float theta )
{
	return tan( theta / 360.0 * TAU );
}

// rotate a vector by theta degrees
vect2 v_rotate( vect2 in, float theta)
{
	vect2 out;
	float thrad = theta / 360.0 * TAU;

	out.x = in.x * cos( thrad ) - in.y * sin( thrad );
	out.y = in.x * sin( thrad ) + in.y * cos( thrad );

	return out;
}

vect2 v_rotate_2( 	vect2 in, 
					vect2 theta)	// padded float
{
	vect2 out;
	float thrad = theta.x / 360.0 * TAU;

	out.x = in.x * cos( thrad ) - in.y * sin( thrad );
	out.y = in.x * sin( thrad ) + in.y * cos( thrad );

	return out;
}

vect2 v_set( float x, float y )
{
	vect2 w;

	w.x = x; 
	w.y = y;

	return w;
}

// returns a vector 90 degrees to the left of input vector
// equivalent to complex multiply by i
vect2 v_complement( vect2 v )
{
	vect2 w;

	w.x = -v.y;
	w.y =  v.x;
	return w;
}


float v_magnitude( vect2 v )
{
	return(sqrt(v.x*v.x + v.y*v.y));
}


vect2 v_normalize( vect2 v )
{
	vect2 w;
	float m = v_magnitude(v);
	
	if(m == 0.0)
	{
		w.x = 0.0;	
		w.y = 0.0;
		
	}
	else
	{
		w.x = v.x / m;
		w.y = v.y / m;
	}
	return w;
}

vect2 v_add( vect2 a, vect2 b )
{	
	vect2 w;

	w.x = a.x + b.x;
	w.y = a.y + b.y;
	return w;
}

// can be used to create radial or concentric vector fields
vect2 v_subtract( vect2 a, vect2 b )
{	
	vect2 w;

	w.x = a.x - b.x;
	w.y = a.y - b.y;
	return w;
}

// scalar multiply vector
vect2 v_scale( vect2 in, float s )
{
	in.x *= s;
	in.y *= s;
	return in;
}

vect2 v_complex_power( vect2 in, float p )
{
	in = v_radial( in );
	in.R = pow( in.R, p );
	in.THETA *= p;
	return in;
}

vect2 v_inverse_square( vect2 in, float diameter, float soften )
{
	vect2 out;
	float mag;

	if( ( in.x == 0.0) && ( in.y == 0.0 ) ) {
		return v_zero;
	}
	else {
		mag = diameter * diameter * sqrt( 1.0 / ( in.x * in.x + in.y * in.y + soften * soften ) );
		out = v_normalize( in );
		out.x *= mag; out.y *= mag;
		return out;
	}
}

// multiplies only x component
vect2 v_multiply_x( vect2 a,	// padded float
					vect2 b )	// padded float
{
	a.x *= b.x;
	return a;
}

// multiplies only y component by b.x
vect2 v_multiply_y( vect2 a,	// padded float
					vect2 b )	// padded float
{
	a.y *= b.x;
	return a;
}

// adds b.x to a.y
vect2 v_add_y( 	vect2 a,	// padded float
				vect2 b )	// padded float
{
	a.y += b.x;
	return a;
}

// Chooses a random point within 3D box - assumes srand() has already been called
vect2 box_of_random2( vect2 min, vect2 max)
{
	vect2 w;
	float xprop = rand1();
	float yprop = rand1();

	w.x = min.x * (1.0 - xprop) + max.x * xprop;
	w.y = min.y * (1.0 - yprop) + max.y * yprop;

	return w;
}

// Returns true if vector v is within a certain distance of a bounding box defined by min and max
bool in_bounds( vect2 v, vect2 min, vect2 max, float dist)
{
	bool inx,iny;

	if( min.x < max.x) {
		inx = ( v.x >= min.x - dist ) &&  ( v.x <= max.x + dist );
	}
	else {
		inx = ( v.x >= max.x - dist  ) &&  ( v.x <= min.x + dist );
	}

	if( min.y < max.y) {
		iny = ( v.y >= min.y - dist  ) &&  ( v.y <= max.y + dist );
	}
	else {
		iny = ( v.y >= max.y - dist  ) &&  ( v.y <= min.y + dist );
	}

	return( inx && iny);
}

vect2 v_radial( vect2 v )
{
	vect2 rad;

	rad.R = v_magnitude( v );
	rad.THETA = vtoa( v );

	return rad;
}

vect2 v_cartesian( vect2 rad )
{
	vect2 v;

	v.x = rad.R * cos_deg( rad.THETA );
	v.y = rad.R * sin_deg( rad.THETA );

	return v;
}



