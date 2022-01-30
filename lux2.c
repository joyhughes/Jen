#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

//#include "vect2.h"
//#include "func_node.h"

#define TAU 6.283185307179586

typedef struct Vect2 
{

	float x,y;

} vect2;

vect2 v_zero;	// yes, it's a global

typedef vect2 (*vect_func)( vect2, vect2 );

typedef struct func_node func_node;

struct func_node {

	bool leaf;
	vect2 leaf_val;
	vect_func fn;
	func_node *a;
	func_node *b;

};

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
vect2 v_complement_2( 	vect2 a, 
						vect2 b )	//ignored
{
	vect2 w;

	w.x = -a.y;
	w.y =  a.x;
	return w;
}

vect2 v_add( vect2 a, vect2 b )
{	
	vect2 w;

	w.x = a.x + b.x;
	w.y = a.y + b.y;
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

// can be used to create radial or concentric vector fields
vect2 v_subtract( vect2 a, vect2 b )
{	
	vect2 w;

	w.x = a.x - b.x;
	w.y = a.y - b.y;
	return w;
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

// ******************* Fnode functions ******************* 

vect2 fnode_eval( func_node *fnode )
{
	if( fnode == NULL ) return v_zero;
	printf("fnode_eval: leaf_val= ( %0.1f, %0.1f )\n", fnode->leaf_val.x, fnode->leaf_val.y );
	if( fnode->leaf ) return fnode->leaf_val;
	else return fnode->fn( fnode_eval( fnode->a ), fnode_eval( fnode->b ) );
}

// Initialized a leaf node
void fnode_init_leaf( func_node* fnode, vect2 leaf_val )
{
	fnode->leaf = true;
	fnode->leaf_val = leaf_val;
	fnode->fn = NULL;
	fnode->a = NULL;
	fnode->b = NULL;	
}

// Initializes a branch node
void fnode_init( func_node* fnode, vect_func fn, func_node *a, func_node *b )
{
	fnode->leaf = false;
	fnode->leaf_val = v_set( 0.0, 0.0 );
	fnode->fn = fn;
	fnode->a = a;
	fnode->b = b;	
}

int main( int argc, char *argv[] )
{
	vect2 result;
	v_zero = v_set( 0.0, 0.0 );

	vect2 v_unitx = v_set( 1.0, 0.0 );


	func_node center; 			fnode_init_leaf( &center, v_zero );

	func_node position;			fnode_init_leaf( &position, v_set( 0.3, 0.5) );

	func_node time;				fnode_init_leaf( &time, v_zero );

	func_node unitx;			fnode_init_leaf( &unitx, v_unitx );

	func_node radial;			fnode_init( &radial, &v_subtract, &position, &center );

	func_node vortex;			fnode_init( &vortex, &v_complement_2, &radial, NULL );

	func_node radial_normal;	fnode_init( &radial_normal, &v_normalize_2, &radial, NULL );

	func_node sin_t;			fnode_init( &sin_t, &sin_ab, &time, &unitx );

	func_node vortex_sin;		fnode_init( &vortex_sin, &v_scale_2, &vortex, &sin_t );

	func_node wiggle;			fnode_init( &wiggle, &v_add, &vortex_sin, &radial_normal );

	func_node wiggle_normal;	fnode_init( &wiggle_normal, &v_normalize_2, &wiggle, NULL );

	result = fnode_eval( &wiggle_normal );

	printf( "result = ( %0.4f, %0.4f )\n", result.x, result.y );
	return 0;

} 