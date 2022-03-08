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

func_data fd_bool( bool b ) 		{ func_data fd;		fd.b = b;	return fd; }
func_data fd_int( int i ) 			{ func_data fd;		fd.i = i;	return fd; }
func_data fd_float( float f ) 		{ func_data fd;		fd.f = f;	return fd; }
func_data fd_vect2( vect2 v ) 		{ func_data fd;		fd.v = v;	return fd; }
func_data fd_frgb( frgb c ) 		{ func_data fd;		fd.c = c;	return fd; }
func_data fd_array( func_data *a )	{ func_data fd;		fd.a = a;	return fd; }

// ********************** Evaluation function ********************** 

func_data fnode_eval( func_node *fnode )
{
	if( fnode->evaluated ) {
		return fnode->value;
	}
	else {
		//printf( "fnode_eval: " );
		//fnode_print( fnode );
		fnode->evaluated = true;
		switch( fnode->nargs ) {

			case 0: return fnode->value;

			case 1: { 	
						fnode->value = fnode->fn.gf1( fnode_eval( fnode->a ) ); 
						return fnode->value;
					}

			case 2: { 	
					fnode->value = fnode->fn.gf2( fnode_eval( fnode->a ), fnode_eval( fnode->b ) );
					return fnode->value;
					}

			case 3: {
				if( fnode->choose ) {					
					if( fnode_eval( fnode->a ).b ) {
						fnode->value = fnode_eval( fnode->b );
						return fnode->value;
					}
					else {
						fnode->value = fnode_eval( fnode->c );
						return fnode->value;
					}
				}
				else { 	
					fnode->value = fnode->fn.gf3( fnode_eval( fnode->a ), fnode_eval( fnode->b ), fnode_eval( fnode->c ) ); 
					return fnode->value;
				}
			}
		}
	}
	return fnode->value;
}

// ********************** Initialization functions ********************** 

// Initialized a leaf node
void fnode_init_leaf( func_node* fnode, func_data value, func_type type )
{
	fnode->evaluated = false;
	fnode->value = value;
	fnode->type = type;
	fnode->choose = false;
	fnode->nargs = 0;
	fnode->fn.gf1 = NULL;
	fnode->a = NULL;
	fnode->b = NULL;	
	fnode->c = NULL;
}

// Initializes a 1 argument node
void fnode_init_1( func_node* fnode, gen_func_1 fn1, func_node *a, func_type type )
{
	printf(" fnode_init_1: %s\n", fnode->name);
	if( a == NULL ) printf( "node a NULL\n");
	fnode->evaluated = false;
	fnode->value = fd_int( 0 );
	fnode->type = type;
	fnode->choose = false;
	fnode->nargs = 1;
	fnode->fn.gf1 = fn1;
	fnode->a = a;
	fnode->b = NULL;
	fnode->c = NULL;	
}

// Initializes a 2 argument node
void fnode_init_2( func_node* fnode, gen_func_2 fn2, func_node *a, func_node *b, func_type type )
{
	printf(" fnode_init_2: %s\n", fnode->name);
	if( a == NULL ) printf( "node a NULL\n");
	if( b == NULL ) printf( "node b NULL\n");
	fnode->evaluated = false;
	fnode->value = fd_int( 0 );
	fnode->type = type;
	fnode->choose = false;
	fnode->nargs = 2;
	fnode->fn.gf2 = fn2;
	fnode->a = a;
	fnode->b = b;
	fnode->c = NULL;	
}

// Initializes a 3 argument node
void fnode_init_3( func_node* fnode, gen_func_3 fn3, func_node *a, func_node *b, func_node *c, func_type type, bool choose )
{
	printf(" fnode_init_3: %s\n", fnode->name);
	if( a == NULL ) printf( "node a NULL\n");
	if( b == NULL ) printf( "node b NULL\n");
	if( c == NULL ) printf( "node c NULL\n");
	fnode->evaluated = false;
	fnode->value = fd_int( 0 );
	fnode->type = type;
	fnode->choose = choose;
	fnode->nargs = 3;
	fnode->fn.gf3 = fn3;
	fnode->a = a;
	fnode->b = b;
	fnode->c = c;	
}

void fnode_set_name( func_node* fnode, const char *name)
{
	sprintf( fnode->name, "%s", name );
}

// ********************** Sanity check ********************** 

void fnode_print( func_node* fnode )
{
	switch( fnode->type ) {
		case FN_BOOL  :	printf( "bool  " ); break;
		case FN_INT   :	printf( "int   " ); break;
		case FN_FLOAT :	printf( "float " ); break;
		case FN_VECT2 :	printf( "vect2 " ); break;
		case FN_FRGB  :	printf( "frgb  " ); break;
   		default : printf( "type undefined ");
   	}

   	printf( "%s ( ", fnode->name );

   	switch( fnode->nargs ) {
   		case 0:
			switch( fnode->type ) {
				case FN_BOOL  :	if( fnode->value.b ) printf( "true )\n" ); else printf( "false )\n"); break;
				case FN_INT   :	printf( "%d )\n", fnode->value.i ); break;
				case FN_FLOAT :	printf( "%f )\n", fnode->value.f ); break;
				case FN_VECT2 :	printf( "[ %f, %f ] )\n", fnode->value.v.x, fnode->value.v.y ); break;
				case FN_FRGB  :	printf( "[ %f, %f, %f ] )\n", fnode->value.c.r, fnode->value.c.g, fnode->value.c.b ); break;
		   		default : printf( "()\n");
		   	}
		   	break;
		case 1: printf( "%s )\n", fnode->a->name ); break;
   		case 2: printf( "%s, %s )\n", fnode->a->name, fnode->b->name ); break;
   		case 3: printf( "%s, %s, %s )\n", fnode->a->name, fnode->b->name, fnode->c->name ); break;
   	}
}

// ********************** copy function ********************** 

// copies everything but subnode pointers - those are handled in ftree_copy()
void fnode_copy( func_node *in, func_node *out )
{
	out->value 		= in->value;
	out->type 		= in->type;
	out->choose 	= in->choose;
	out->nargs 		= in->nargs;
	out->fn 		= in->fn;		
	sprintf( out->name, "%s", in->name );
} 

// ********************** I/O functions ********************** 

void fnode_load( FILE *fp, func_node *fnode, func_tree *ftree, char *junk, bool *eof )
{
	char buffer[ 255 ];
	func_node *fn_a;
	func_node *fn_b;
	func_node *fn_c;
	gen_func gf;
	bool b;
	int i;
	float f;
	vect2 v;
	frgb c;
	int n;
	func_type ftype;
	bool choose;

	printf(" fnode_load: %s\n", fnode->name );
	file_get_string( fp, buffer, junk, eof );
	if( !(*eof) ) {
		if( !strcmp( buffer, "bool") ) { 
			b = file_get_bool(  fp, buffer, junk, eof );
			fnode_init_leaf( fnode, fd_bool( b ), FN_BOOL );
		}
		else if( !strcmp( buffer, "int") ) {
			i = file_get_int(  fp, buffer, junk, eof );
			fnode_init_leaf( fnode, fd_int( i ), FN_INT );
		}
		else if( !strcmp( buffer, "float") ) {	
			f = file_get_float(  fp, buffer, junk, eof );
			fnode_init_leaf( fnode, fd_float( f	), FN_FLOAT );
		}
		else if( !strcmp( buffer, "vect2") ) { 	
			v = file_get_vect2(  fp, buffer, junk, eof );
			fnode_init_leaf( fnode, fd_vect2( v	), FN_VECT2 );
		}
		else if( !strcmp( buffer, "frgb") ) {	
			c = file_get_frgb(  fp, buffer, junk, eof );
			fnode_init_leaf( fnode, fd_frgb( c ), FN_FRGB );
		}

		else  {
			ftype = get_gen_func( buffer, &gf, &n, &choose );
			file_get_string( fp, buffer, junk, eof );
			fn_a = ftree_index( ftree, buffer );

			if( n == 1 ) {
				fnode_init_1( fnode, gf.gf1, fn_a, ftype );
			}
			else {
				file_get_string( fp, buffer, junk, eof );
				fn_b = ftree_index( ftree, buffer );
				if( n==2 ) {
					fnode_init_2( fnode, gf.gf2, fn_a, fn_b, ftype );
				}
				else {
					file_get_string( fp, buffer, junk, eof );
					fn_c = ftree_index( ftree, buffer );
					fnode_init_3( fnode, gf.gf3, fn_a, fn_b, fn_c, ftype, choose );
				}
			}
		}
		fnode_print( fnode );
	}
}

// ********************** Calculation functions ********************** 
// Organized by type and parameter

// ********************** bool ( bool ) ********************** 

// Identity function
func_data fn_b_copy( func_data f )
{
	return f;
}

// ********************** bool ( int ) ********************** 

func_data fn_b_even( func_data f1 )
{
	f1.b = ( ( f1.i ) % 2 ) == 0;
	return f1;
}

// ********************** bool ( float, float ) ********************** 

func_data fn_b_less_than( func_data f1, func_data f2 )
{
	f1.b = f1.f < f2.f;
	return f1;
}

// ********************** int ( int ) ********************** 

// Identity function
func_data fn_i_copy( func_data f )
{
	return f;
}

// ********************** int ( int, int ) ********************** 

func_data fn_i_div( func_data i1, func_data i2 )
{
	i1.i = i1.i / i2.i;
	return i1;
}

// ********************** int ( float ) ********************** 

func_data fn_i_trunc( func_data f1 )
{
	f1.i = f1.f;
	return f1;
}

// ********************** int ( float, float ) ********************** 

func_data fn_i_f_div_f( func_data f1, func_data f2 )
{
	f1.i = f1.f / f2.f;
	return f1;
}

// ********************** float ( float ) ********************** 

// Identity function
func_data fn_f_copy( func_data f )
{
	return f;
}

// ********************** float ( vect2 ) ********************** 

func_data fn_f_magnitude( func_data v )
{
	v.f = v_magnitude( v.v );
	return v;
}

func_data fn_f_vtoa( func_data v )
{
	v.f = vtoa( v.v );
	return v;
}

// ********************** float ( float, int ) ********************** 

func_data fn_f_div_i( func_data f, func_data i )
{
	f.f /= i.i;
	return f;
}

func_data fn_f_mult_i( func_data f, func_data i )
{
	f.f *= i.i;
	return f;
}

// ********************** float ( float, float ) ********************** 

func_data fn_f_add( func_data f1, func_data f2 )
{
	f1.f += f2.f;
	return f1;
}

func_data fn_f_subtract( func_data f1, func_data f2 )
{
	f1.f -= f2.f;
	return f1;
}

func_data fn_f_multiply( func_data f1, func_data f2 )
{
	f1.f *= f2.f;
	return f1;
}

func_data fn_f_divide( func_data f1, func_data f2 )
{
	f1.f /= f2.f;
	return f1;
}

func_data fn_f_sin_ab( func_data f1, func_data f2 )
{
	f1.f = sin_deg( f1.f * f2.f );
	return f1;
}

func_data fn_f_mod( func_data f1, func_data f2 )
{
	f1.f = f1.f - floor( f1.f / f2.f ) * f2.f;
	return f1;
}

// ********************** float ( bool, float, float ) ********************** 

func_data fn_f_choose( func_data f1, func_data f2, func_data f3 )
{
	if( f1.b ) return f2; 
	else return f3;
}

// ********************** float ( float, float, bool ) ********************** 

func_data fn_f_kaleido( func_data f, func_data width, func_data reflect )
{
	int segment = f.f / width.f;
	f.f -= width.f * segment;
	if( reflect.b && ( segment % 2 ) ) f.f = width.f - f.f;

	return f;
}

// ********************** float ( float, float, float ) ********************** 

func_data fn_f_inverse_square( func_data f, func_data diameter, func_data soften )
{
	f.f = diameter.f * diameter.f * sqrt( 1.0 / ( f.f * f.f + soften.f * soften.f ) );
	return f;
}

func_data fn_f_a_sin_bc( func_data a, func_data b, func_data c )
{
	a.f = a.f * sin_deg( b.f * c.f );
	return a;
}

// ********************** vect2 ( vect2 ) ********************** 

// Identity function
func_data fn_v_copy( func_data f )
{
	return f;
}

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

func_data fn_v_squaradial( func_data v )
{
	vect2 rad;

	rad.THETA = vtoa( v.v );
	if( 		( rad.THETA < 45.0 )  || ( rad.THETA > 315.0 ) ) rad.R =  v.v.x;
	else if( 	( rad.THETA > 45.0 )  && ( rad.THETA < 135.0 ) ) rad.R =  v.v.y;
	else if( 	( rad.THETA > 135.0 ) && ( rad.THETA < 225.0 ) ) rad.R = -v.v.x;
	else rad.R = -v.v.y;

	return fd_vect2( rad );
}

func_data fn_v_squartesian( func_data v )
{
	vect2 cart;

	v.v.THETA = fmod( v.v.THETA, 360.0 );

	if( ( v.v.THETA < 45.0 ) || ( v.v.THETA > 315.0) ) { 		cart.x =  v.v.R;	cart.y =  v.v.R * tan_deg( v.v.THETA ); }
	else if( ( v.v.THETA > 45.0 ) && ( v.v.THETA < 135.0 ) ) { 	cart.y =  v.v.R;	cart.x = -v.v.R * tan_deg( v.v.THETA - 90.0 ); }
	else if( ( v.v.THETA > 135.0 ) && ( v.v.THETA < 225.0 ) ) { cart.x = -v.v.R; 	cart.y = -v.v.R * tan_deg( v.v.THETA - 180.0 ); }
	else {														cart.y = -v.v.R;	cart.x =  v.v.R * tan_deg( v.v.THETA - 270.0 ); }

	return fd_vect2( cart );
}

func_data fn_v_complement( func_data v )
{
	v.v = v_complement( v.v );
	return v;
}

func_data fn_v_normalize( func_data v )
{
	v.v = v_normalize( v.v );
	return v;
}

// ********************** vect2 ( float, float ) ********************** 

func_data fn_v_cartesian_f( func_data r, func_data theta )
{
	vect2 v = v_set( r.f, theta.f );
	r.v = v_cartesian( v );
	return r;
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

func_data fn_v_scale( func_data v, func_data f )
{
	v.v.x *= f.f;
	v.v.y *= f.f;
	return v;
}

func_data fn_v_rotate( func_data v, func_data ang )
{
	v.v = v_rotate( v.v, ang.f );
	return v;
}

func_data fn_v_complex_power( func_data v, func_data p )
{
	v.v = v_complex_power( v.v, p.f );
	return v;
}


// ********************** vect2 ( vect2, vect2 ) ********************** 

func_data fn_v_add( func_data v1, func_data v2 )
{
	v1.v.x += v2.v.x;
	v1.v.y += v2.v.y;
	return v1;
}

func_data fn_v_subtract( func_data v1, func_data v2 )
{
	v1.v.x -= v2.v.x;
	v1.v.y -= v2.v.y;
	return v1;
}

// ********************** vect2 ( bool, vect2, vect2 ) ********************** 

func_data fn_v_choose( func_data f1, func_data f2, func_data f3 )
{
	if( f1.b ) return f2; 
	else return f3;
}

// ********************** vect2 ( vect2, float, bool ) ********************** 

func_data fn_v_kaleido( func_data v, func_data width, func_data reflect )
{
	int segment = v.v.THETA / width.f;
	v.v.THETA -= width.f * segment;
	if( reflect.b && ( segment % 2 ) ) v.v.THETA = width.f - v.v.THETA;

	return v;
}

// ********************** frgb ( float ) ********************** 

func_data fn_c_rainbow( func_data f )
{
	f.c = rainbow( f.f );
	return f;
}

// ********************** frgb ( frgb ) ********************** 

// Identity function
func_data fn_c_copy( func_data f )
{
	return f;
}

// ********************** Ftree functions ********************** 

void ftree_init( func_tree *ftree, int n )
{
	int i;

	ftree->n = n;
	ftree->nodes = (func_node *)malloc( n * sizeof(func_node) );
	func_node *fnode = ftree->nodes;

	// fill tree with false leaf nodes to clear memory
	for( i=0; i<n; i++ ) {
		fnode_init_leaf( fnode, fd_bool( false ), FN_BOOL );
		fnode++;
	}
}

void ftree_set_name( func_tree *ftree, const char *name )
{
	sprintf( ftree->name, "%s", name );
}

char* ftree_get_name( func_tree *ftree )
{
	return ftree->name;
}

// generative function lookup table
func_type get_gen_func ( const char* name, gen_func *gf, int *n, bool *choose )
{
	*choose = false;


	// ********************** BOOL ********************** 

	// ********************** bool ( bool ) ********************** 
	if( !strcmp( name, "fn_b_copy" ) )			{ 	gf->gf1 = &fn_b_copy;			*n = 1;		return FN_BOOL;  }

	// ********************** bool ( int ) ********************** 
	if( !strcmp( name, "fn_b_even" ) )			{ 	gf->gf1 = &fn_b_even;			*n = 1;		return FN_BOOL;  }

	// ********************** bool ( float, float ) ********************** 
	if( !strcmp( name, "fn_b_less_than" ) )		{ 	gf->gf2 = &fn_b_less_than;		*n = 2;		return FN_BOOL;  }

	// ********************** INT ********************** 

	// ********************** int ( int ) ********************** 
	if( !strcmp( name, "fn_i_copy" ) )			{ 	gf->gf1 = &fn_i_copy;			*n = 1;		return FN_INT;  }	

	// ********************** int ( int, int ) ********************** 
	if( !strcmp( name, "fn_i_div" ) )			{ 	gf->gf2 = &fn_i_div;			*n = 2;		return FN_INT;  }	

	// ********************** int ( float ) ********************** 
	if( !strcmp( name, "fn_i_trunc" ) )			{ 	gf->gf1 = &fn_i_trunc;			*n = 1;		return FN_INT;  }	

	// ********************** int ( float, float ) ********************** 
	if( !strcmp( name, "fn_i_f_div_f" ) )		{ 	gf->gf2 = &fn_i_f_div_f;		*n = 2;		return FN_INT;  }	

	// ********************** FLOAT ********************** 

	// ********************** float ( float ) ********************** 
	if( !strcmp( name, "fn_f_copy" ) )			{ 	gf->gf1 = &fn_f_copy;			*n = 1;		return FN_FLOAT;  }	

	// ********************** float ( vect2 ) ********************** 
	if( !strcmp( name, "fn_f_magnitude" ) )		{ 	gf->gf1 = &fn_f_magnitude;		*n = 1;		return FN_FLOAT;  }	
	if( !strcmp( name, "fn_f_vtoa" ) )			{ 	gf->gf1 = &fn_f_vtoa;			*n = 1;		return FN_FLOAT;  }	

	// ********************** float ( float, int ) ********************** 
	if( !strcmp( name, "fn_f_div_i" ) )			{ 	gf->gf2 = &fn_f_div_i;			*n = 2;		return FN_FLOAT; }
	if( !strcmp( name, "fn_f_mult_i" ) )		{ 	gf->gf2 = &fn_f_mult_i;			*n = 2;		return FN_FLOAT; }

	// ********************** float ( float, float ) ********************** 
	if( !strcmp( name, "fn_f_subtract" ) )		{ 	gf->gf2 = &fn_f_subtract;		*n = 2;		return FN_FLOAT; }
	if( !strcmp( name, "fn_f_add" ) )			{ 	gf->gf2 = &fn_f_add;			*n = 2;		return FN_FLOAT; }
	if( !strcmp( name, "fn_f_multiply" ) )		{ 	gf->gf2 = &fn_f_multiply;		*n = 2;		return FN_FLOAT; }
	if( !strcmp( name, "fn_f_divide" ) )		{ 	gf->gf2 = &fn_f_divide;			*n = 2;		return FN_FLOAT; }
	if( !strcmp( name, "fn_f_sin_ab" ) )		{ 	gf->gf2 = &fn_f_sin_ab;			*n = 2;		return FN_FLOAT; }
	if( !strcmp( name, "fn_f_mod" ) )			{ 	gf->gf2 = &fn_f_mod;			*n = 2;		return FN_FLOAT; }	

	// ********************** float ( bool, float, float ) ********************** 
	if( !strcmp( name, "fn_f_choose" ) )		{ 	gf->gf3 = &fn_f_choose;			*n = 3;		*choose = true;		return FN_FLOAT; }

	// ********************** float ( float, float, bool ) ********************** 
	if( !strcmp( name, "fn_f_kaleido" ) )		{ 	gf->gf3 = &fn_f_kaleido;		*n = 3;		return FN_FLOAT; }

	// ********************** float ( float, float, float ) ********************** 
	if( !strcmp( name, "fn_f_inverse_square" ) ){ 	gf->gf3 = &fn_f_inverse_square;	*n = 3;		return FN_FLOAT; }
	if( !strcmp( name, "fn_f_a_sin_bc" ) )		{ 	gf->gf3 = &fn_f_a_sin_bc;		*n = 3;		return FN_FLOAT; }

	// ********************** VECT2 ********************** 

	// ********************** vect2 ( vect2 ) ********************** 
	if( !strcmp( name, "fn_v_copy" ) ) 			{	gf->gf1 = &fn_v_copy;			*n = 1;		return FN_VECT2; }
	if( !strcmp( name, "fn_v_radial" ) ) 		{	gf->gf1 = &fn_v_radial;			*n = 1;		return FN_VECT2; }
	if( !strcmp( name, "fn_v_cartesian" ) )		{ 	gf->gf1 = &fn_v_cartesian;		*n = 1;		return FN_VECT2; }
	if( !strcmp( name, "fn_v_squaradial" ) )	{ 	gf->gf1 = &fn_v_squaradial;		*n = 1;		return FN_VECT2; }
	if( !strcmp( name, "fn_v_squartesian" ) )	{ 	gf->gf1 = &fn_v_squartesian;	*n = 1;		return FN_VECT2; }
	if( !strcmp( name, "fn_v_complement" ) )	{ 	gf->gf1 = &fn_v_complement;		*n = 1;		return FN_VECT2; }
	if( !strcmp( name, "fn_v_normalize" ) )		{ 	gf->gf1 = &fn_v_normalize;		*n = 1;		return FN_VECT2; }

	// ********************** vect2 ( float, float ) ********************** 
	if( !strcmp( name, "fn_v_cartesian_f" ) )	{	gf->gf2 = &fn_v_cartesian_f;	*n = 2;		return FN_VECT2; }	

	// ********************** vect2 ( vect2, float ) ********************** 
	if( !strcmp( name, "fn_v_add_y" ) )			{	gf->gf2 = &fn_v_add_y;			*n = 2;		return FN_VECT2; }	
	if( !strcmp( name, "fn_v_multiply_y" ) )	{	gf->gf2 = &fn_v_multiply_y;		*n = 2;		return FN_VECT2; } 
	if( !strcmp( name, "fn_v_scale" ) )			{	gf->gf2 = &fn_v_scale;			*n = 2;		return FN_VECT2; } 
	if( !strcmp( name, "fn_v_rotate" ) )		{	gf->gf2 = &fn_v_rotate;			*n = 2;		return FN_VECT2; } 
	if( !strcmp( name, "fn_v_complex_power" ) )	{	gf->gf2 = &fn_v_complex_power;	*n = 2;		return FN_VECT2; } 

	// ********************** vect2 ( vect2, vect2 ) ********************** 
	if( !strcmp( name, "fn_v_add" ) )			{	gf->gf2 = &fn_v_add;			*n = 2;		return FN_VECT2; } 
	if( !strcmp( name, "fn_v_subtract" ) )		{	gf->gf2 = &fn_v_subtract;		*n = 2;		return FN_VECT2; } 

	// ********************** vect2 ( bool, vect2, vect2 ) ********************** 
	if( !strcmp( name, "fn_v_choose" ) )		{ 	gf->gf3 = &fn_v_choose;			*n = 3;		*choose = true;		return FN_VECT2; }

	// ********************** vect2 ( vect2, float, bool ) ********************** 
	if( !strcmp( name, "fn_v_kaleido" ) )		{ 	gf->gf3 = &fn_v_kaleido;		*n = 3;		return FN_VECT2; }

	// ********************** FRGB ********************** 

	// ********************** frgb ( float ) ********************** 
	if( !strcmp( name, "fn_c_rainbow" ) )		{ 	gf->gf1 = &fn_c_rainbow;		*n = 1;		return FN_VECT2; }

	// ********************** frgb ( copy ) ********************** 
	if( !strcmp( name, "fn_c_copy" ) )			{ 	gf->gf1 = &fn_c_copy;			*n = 1;		return FN_VECT2; }

	printf( "get_gen_func: function %s not found\n", name );
	return FN_NULL;
}

void ftree_load( func_tree *ftree, const char *filename )
{	
	FILE *fp;
	bool eof = false;		// end of file or other error detected
	char buffer[255];
	char junk[1024];
	int node = 0;
	int n;

	fp = fopen( filename, "r" );
	while( !eof ) { 			
		file_get_string( fp, buffer, junk, &eof );
		if( !eof ) {
			if( !strcmp( buffer, "n") ) {
				n = file_get_int( fp, buffer, junk, &eof );
				ftree_init( ftree, n );
			}		
			else if( !strcmp( buffer, "name") ) {
				file_get_string( fp, ftree->name, junk, &eof );
			}
			else {
				fnode_set_name( &(ftree->nodes[ node ]), buffer );
				fnode_load( fp,  &(ftree->nodes[ node ]), ftree, junk, &eof );
				node++;
				if( node == n ) eof = true;
			}
		}
	}
	fclose( fp );
	ftree_print( ftree );
}

func_node* ftree_index( func_tree *ftree, const char *name )
{
	int i;
	func_node *fnode = ftree->nodes;

	for( i=0; i<ftree->n; i++ )
	{
		if( !strcmp( name, fnode->name) ) return fnode;
		fnode++;
	}
	printf("ftree_index - node %s not found\n", name );
	return NULL;	// name not found
}

void ftree_print( func_tree *ftree )
{
	int i;
	func_node *fnode = ftree->nodes;

	printf( "Function tree %s\n", ftree->name );
	printf( " %d nodes\n", ftree->n );
	for( i=0; i<ftree->n; i++ ) {
		fnode_print( fnode );
		fnode++;
	}
}

void ftree_reset( func_tree *ftree )
{
	int i;
	func_node *fnode = ftree->nodes;

	for( i=0; i<ftree->n; i++ ) {
		fnode->evaluated = false;
		fnode++;
	}
}

void ftree_copy( func_tree *in, func_tree *out )
{
	int i;

	ftree_init( out, in->n );
	ftree_set_name( out, in->name );

	// copy all nodes
	for( i = 0; i < out->n; i++ ) {
		fnode_copy( &( in->nodes[ i ] ), &( out->nodes[ i ] ) );
	}

	// rebuild tree
	for( i = 0; i < out->n; i++ ) {
		if( out->nodes[ i ].nargs == 0 ) {
			out->nodes[ i ].a = NULL;
			out->nodes[ i ].b = NULL;
			out->nodes[ i ].c = NULL;

		} else if( out->nodes[ i ].nargs == 1) {
			out->nodes[ i ].a = ftree_index( out, in->nodes[ i ].a->name );
			out->nodes[ i ].b = NULL;
			out->nodes[ i ].c = NULL;

		} else if( out->nodes[ i ].nargs == 2) {
			out->nodes[ i ].a = ftree_index( out, in->nodes[ i ].a->name );
			out->nodes[ i ].b = ftree_index( out, in->nodes[ i ].b->name );
			out->nodes[ i ].c = NULL;

		} else if( out->nodes[ i ].nargs == 3) {
			out->nodes[ i ].a = ftree_index( out, in->nodes[ i ].a->name );
			out->nodes[ i ].b = ftree_index( out, in->nodes[ i ].b->name );
			out->nodes[ i ].c = ftree_index( out, in->nodes[ i ].c->name );
		}
	}
}

// ********************** Func_tree_list functions ********************** 

void ftree_list_init( func_tree_list *list, int n )
{
	int i;

	list->n = n;
	list->trees = (func_tree *)malloc( n * sizeof(func_tree) );
}

void ftree_list_set_name( func_tree_list *list, const char *name )
{
	sprintf( list->name, "%s", name );
}

char* ftree_list_get_name( func_tree *list )
{
	return list->name;
}

func_tree* ftree_list_index( func_tree_list *list, const char* name )
{
	int i;
	func_tree *ftree = list->trees;

	for( i = 0; i < list->n; i++ )
	{
		if( !strcmp( name, ftree->name ) ) return ftree;
		ftree++;
	}
	return NULL;	// name not found
}






























