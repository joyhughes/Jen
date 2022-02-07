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
	switch( fnode->nargs ) {
		case 0: return fnode->leaf_val;
		case 1: return fnode->fn.gf1( fnode_eval( fnode->a ) );
		case 2: return fnode->fn.gf2( fnode_eval( fnode->a ), fnode_eval( fnode->b ) );
		case 3: /*if( fnode->choose ) {
					if( fnode_eval( fnode->a ).b ) return fnode_eval( fnode->b );
					else return fnode_eval( fnode->c );
				}
				else  */ return fnode->fn.gf3( fnode_eval( fnode->a ), fnode_eval( fnode->b ), fnode_eval( fnode->c ) );
	}
	printf(" fnode_eval: invalid number of arguments\n");
	return fnode->leaf_val;
}

// ********************** Initialization functions ********************** 

// Initialized a leaf node
void fnode_init_leaf( func_node* fnode, func_data leaf_val, fn_type type )
{
	fnode->leaf_val = leaf_val;
	fnode->type = type;
	fnode->choose = false;
	fnode->nargs = 0;
	fnode->fn.gf1 = NULL;
	fnode->a = NULL;
	fnode->b = NULL;	
	fnode->c = NULL;	
}

// Initializes a 1 argument node
void fnode_init_1( func_node* fnode, gen_func_1 fn1, func_node *a, fn_type type )
{
	printf(" fnode_init_1: %s\n", fnode->name);
	fnode->leaf_val = fd_int( 0 );
	fnode->type = type;
	fnode->choose = false;
	fnode->nargs = 1;
	fnode->fn.gf1 = fn1;
	fnode->a = a;
	fnode->b = NULL;
	fnode->c = NULL;	
}

// Initializes a 2 argument node
void fnode_init_2( func_node* fnode, gen_func_2 fn2, func_node *a, func_node *b, fn_type type )
{
	printf(" fnode_init_2: %s\n", fnode->name);
	fnode->leaf_val = fd_int( 0 );
	fnode->type = type;
	fnode->choose = false;
	fnode->nargs = 2;
	fnode->fn.gf2 = fn2;
	fnode->a = a;
	fnode->b = b;
	fnode->c = NULL;	
}

// Initializes a 3 argument node
void fnode_init_3( func_node* fnode, gen_func_3 fn3, func_node *a, func_node *b, func_node *c, fn_type type, bool choose )
{
	printf(" fnode_init_3: %s\n", fnode->name);
	fnode->leaf_val = fd_int( 0 );
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
				case FN_BOOL  :	if( fnode->leaf_val.b ) printf( "true )\n" ); else printf( "false )\n"); break;
				case FN_INT   :	printf( "%d )\n", fnode->leaf_val.i ); break;
				case FN_FLOAT :	printf( "%f )\n", fnode->leaf_val.f ); break;
				case FN_VECT2 :	printf( "[ %f, %f ] )\n", fnode->leaf_val.v.x, fnode->leaf_val.v.y ); break;
				case FN_FRGB  :	printf( "[ %f, %f, %f ] )\n", fnode->leaf_val.c.r, fnode->leaf_val.c.g, fnode->leaf_val.c.b ); break;
		   		default : printf( "()\n");
		   	}
		   	break;
		case 1: printf( "%s )\n", fnode->a->name ); break;
   		case 2: printf( "%s, %s )\n", fnode->a->name, fnode->b->name ); break;
   		case 3: printf( "%s, %s, %s )\n", fnode->a->name, fnode->b->name, fnode->c->name ); break;
   	}
}

// ********************** bool ( int ) ********************** 

func_data fn_b_even( func_data f1 )
{
	if (f1.i % 2) f1.b = true;
	else f1.b = false;
	return f1;
}

// ********************** bool ( float, float ) ********************** 

func_data fn_b_less_than( func_data f1, func_data f2 )
{
	f1.b = f1.f < f2.f;
	return f1;
}

// ********************** int ( float ) ********************** 

func_data fn_i_trunc( func_data f1 )
{
	f1.i = f1.f;
	return f1;
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

func_data fn_f_sin_ab( func_data f1, func_data f2 )
{
	f1.f = sin_deg( f1.f * f2.f );
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

// ********************** Ftree functions ********************** 

void ftree_init( func_tree *ftree, int n )
{
	ftree->n = n;
	ftree->nodes = (func_node *)malloc( n * sizeof(func_node) );
}

void ftree_set_name( func_tree *ftree, const char *name )
{
	sprintf( ftree->name, "%s", name );
}

char* ftree_get_name( func_tree *ftree )
{
	return ftree->name;
}

fn_type get_gen_func ( const char* name, gen_func *gf, int *n, bool *choose )
{
	// ********************** bool ( int ) ********************** 
	if( !strcmp( name, "fn_b_even" ) )			{ 	gf->gf1 = &fn_b_even;			*n = 1;	*choose=false;	return FN_BOOL;  }

	// ********************** bool ( float, float ) ********************** 
	if( !strcmp( name, "fn_b_less_than" ) )		{ 	gf->gf2 = &fn_b_less_than;		*n = 2;	*choose=false;	return FN_BOOL;  }

	// ********************** int ( float ) ********************** 
	if( !strcmp( name, "fn_i_trunc" ) )			{ 	gf->gf1 = &fn_i_trunc;			*n = 1;	*choose=false;	return FN_INT;  }	

	// ********************** float ( vect2 ) ********************** 
	if( !strcmp( name, "fn_f_magnitude" ) )		{ 	gf->gf1 = &fn_f_magnitude;		*n = 1;	*choose=false;	return FN_FLOAT;  }	
	if( !strcmp( name, "fn_f_vtoa" ) )			{ 	gf->gf1 = &fn_f_vtoa;			*n = 1;	*choose=false;	return FN_FLOAT;  }	

	// ********************** float ( float, int ) ********************** 
	if( !strcmp( name, "fn_f_div_i" ) )			{ 	gf->gf2 = &fn_f_div_i;			*n = 2;	*choose=false;	return FN_FLOAT; }

	// ********************** float ( float, float ) ********************** 
	if( !strcmp( name, "fn_f_subtract" ) )		{ 	gf->gf2 = &fn_f_subtract;		*n = 2;	*choose=false;	return FN_FLOAT; }
	if( !strcmp( name, "fn_f_add" ) )			{ 	gf->gf2 = &fn_f_add;			*n = 2;	*choose=false;	return FN_FLOAT; }
	if( !strcmp( name, "fn_f_multiply" ) )		{ 	gf->gf2 = &fn_f_multiply;		*n = 2;	*choose=false;	return FN_FLOAT; }
	if( !strcmp( name, "fn_f_sin_ab" ) )		{ 	gf->gf2 = &fn_f_sin_ab;			*n = 2;	*choose=false;	return FN_FLOAT; }

	// ********************** float ( bool, float, float ) ********************** 
	if( !strcmp( name, "fn_f_choose" ) )		{ 	gf->gf3 = &fn_f_choose;			*n = 3;	*choose=true;	return FN_FLOAT; }

	// ********************** float ( float, float, bool ) ********************** 
	if( !strcmp( name, "fn_f_kaleido" ) )		{ 	gf->gf3 = &fn_f_kaleido;		*n = 3;	*choose=false;	return FN_FLOAT; }

	// ********************** vect2 ( vect2 ) ********************** 
	if( !strcmp( name, "fn_v_radial" ) ) 		{	gf->gf1 = &fn_v_radial;			*n = 1;	*choose=false;	return FN_VECT2; }
	if( !strcmp( name, "fn_v_cartesian" ) )		{ 	gf->gf1 = &fn_v_cartesian;		*n = 1;	*choose=false;	return FN_VECT2; }
	if( !strcmp( name, "fn_v_squaradial" ) )	{ 	gf->gf1 = &fn_v_squaradial;		*n = 1;	*choose=false;	return FN_VECT2; }
	if( !strcmp( name, "fn_v_squartesian" ) )	{ 	gf->gf1 = &fn_v_squartesian;	*n = 1;	*choose=false;	return FN_VECT2; }

	// ********************** vect2 ( float, float ) ********************** 
	if( !strcmp( name, "fn_v_cartesian_f" ) )	{	gf->gf2 = &fn_v_cartesian_f;	*n = 2;	*choose=false;	return FN_VECT2; }	

	// ********************** vect2 ( vect2, float ) ********************** 
	if( !strcmp( name, "fn_v_add_y" ) )			{	gf->gf2 = &fn_v_add_y;			*n = 2;	*choose=false;	return FN_VECT2; }	
	if( !strcmp( name, "fn_v_multiply_y" ) )	{	gf->gf2 = &fn_v_multiply_y;		*n = 2;	*choose=false;	return FN_VECT2; } 

	// ********************** vect2 ( bool, vect2, vect2 ) ********************** 
	if( !strcmp( name, "fn_v_choose" ) )		{ 	gf->gf3 = &fn_v_choose;			*n = 3;	*choose=true;	return FN_VECT2; }

	// ********************** vect2 ( vect2, float, bool ) ********************** 
	if( !strcmp( name, "fn_v_kaleido" ) )		{ 	gf->gf3 = &fn_v_kaleido;		*n = 3;	*choose=false;	return FN_VECT2; }

	return FN_NULL;
}

void load_node( FILE *fp, func_node *fnode, func_tree *ftree, char *junk, bool *eof )
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
	fn_type ftype;
	bool choose;

	printf(" load_node: %s\n", fnode->name );
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
				load_node( fp,  &(ftree->nodes[ node ]), ftree, junk, &eof );
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

