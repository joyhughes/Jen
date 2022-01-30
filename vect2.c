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

// ******************* Vector functions ******************* 

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

vect2 v_complement_2( 	vect2 a, 
						vect2 b )	//ignored
{
	vect2 w;

	w.x = -a.y;
	w.y =  a.x;
	return w;
}

float v_magnitude( vect2 v )
{
	return(sqrt(v.x*v.x + v.y*v.y));
}

vect2 v_magnitude_2( vect2 a,
					vect2 b )	//ignored
{
	vect2 w;

	w.x = sqrt( a.x * a.x + a.y * a.y );
	w.y = 0.0;
	return( w );				// padded float
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

// Two argument normalize - second argument ignored
vect2 v_normalize_2( 	vect2 a, 
						vect2 b )	// ignored
{
return v_normalize( a );
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
	vect2 out;

	out.x = in.x * s;
	out.y = in.y * s;
	return out;
}

// scale vector linearly 
vect2 v_scale_2( vect2 in, 
				vect2 s )		// padded float
{
	vect2 out;

	out.x = in.x * s.x;
	out.y = in.y * s.x;
	return out;
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

vect2 v_inverse_square_2( 	vect2 in, 
							vect2 soften )	// padded float
{
	vect2 out;
	float mag;

	if( ( in.x == 0.0) && ( in.y == 0.0 ) ) {
		return v_zero;
	}
	else {
		mag = sqrt( 1.0 / ( in.x * in.x + in.y * in.y + soften.x * soften.x ) );
		out = v_normalize( in );
		out.x *= mag; out.y *= mag;
		return out;
	}
}

vect2 sin_ab( 	vect2 a,	// padded float
				vect2 b )	// padded float
{
	vect2 w;

	w.x = sin( a.x * b.x );
	w.y = 0.0;

	return w;	// padded float
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

