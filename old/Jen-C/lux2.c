#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "vect2.h"
#include "func_node.h"

int main( int argc, char *argv[] )
{
	vect2 result;
	v_zero = v_set( 0.0, 0.0 );

	vect2 v_unitx = v_set( 1.0, 0.0 );


	func_node center; 			fnode_init_leaf( &center, v_zero );

	func_node position;			fnode_init_leaf( &position, v_set( 0.3, 0.6) );

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