#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "vect2.h"
#include "frgb.h"
#include "joy_io.h"
#include "func_node.h"

// ********************** Set func_data functions ********************** 

func_data fd_bool( bool b ) 	{ func_data fd;		fd.b = b;	return fd; }
func_data fd_int( int i ) 		{ func_data fd;		fd.i = i;	return fd; }
func_data fd_float( float f ) 	{ func_data fd;		fd.f = f;	return fd; }
func_data fd_vect2( vect2 v ) 	{ func_data fd;		fd.v = v;	return fd; }
func_data fd_frgb( frgb c ) 	{ func_data fd;		fd.c = c;	return fd; }

// ********************** Evaluation function ********************** 

func_data fnode_eval( func_node const *fnode )
{
	if( fnode->a == NULL ) return fnode->leaf_val;
	else if( fnode->b == NULL ) return fnode->fn1( fnode_eval( fnode->a ) );
	else return fnode->fn2( fnode_eval( fnode->a ), fnode_eval( fnode->b ) );
}

// ********************** Initialization functions ********************** 

// Initialized a leaf node
void fnode_init_leaf( func_node* fnode, func_data leaf_val )
{
	fnode->leaf_val = leaf_val;
	fnode->fn1 = NULL;
	fnode->fn2 = NULL;
	fnode->a = NULL;
	fnode->b = NULL;	
}

// Initializes a 1 argument node
void fnode_init_1( func_node* fnode, gen_func_1 fn1, func_node *a )
{
	fnode->leaf_val = fd_int( 0 );
	fnode->fn1 = fn1;
	fnode->fn2 = NULL;
	fnode->a = a;
	fnode->b = NULL;	
}

// Initializes a 2 argument node
void fnode_init_2( func_node* fnode, gen_func_2 fn2, func_node *a, func_node *b )
{
	fnode->leaf_val = fd_int( 0 );
	fnode->fn1 = NULL;
	fnode->fn2 = fn2;
	fnode->a = a;
	fnode->b = b;	
}

// ********************** float ( float, float ) ********************** 

func_data fn_f_multiply( func_data f1, func_data f2 )
{
	f1.f *= f2.f;
	return f1;
}

func_data fn_f_sin_ab( func_data f1, func_data f2 )
{
	f1.f = sin_deg( f1.f * f2.f );
	return f1;
}

// ********************** vect2 ( vect2 ) ********************** 

func_data fn_v_radial( func_data v )
{
	v.v = v_radial( v.v );
	return v;
}

func_data fn_v_cartesian( func_data v )
{
	v.v = v_cartesian( v.v );
	return v;
}

// ********************** vect2 ( vect2, float ) ********************** 

func_data fn_v_add_y( func_data v, func_data f )
{
	v.v.y += f.f;
	return v;
}

func_data fn_v_multiply_y( func_data v, func_data f )
{
	v.v.y *= f.f;
	return v;
}

