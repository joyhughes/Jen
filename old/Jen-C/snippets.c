	// Where center of sky splats on background
	vect2 sky_center;		sky_center.x = 5.0;				sky_center.y = 6.0;
	float sky_angle;		float sky_angle_min = -20.0;	float sky_angle_max = 20.0;



		sky_angle = sky_angle_min + ( sky_angle_max - sky_angle_min ) / nframes * frame;	// Interpolate sky angle
		fimage_fill( black, &background );
		// splat rotated sky onto background image
		fimage_splat( 
			sky_center, 			// coordinates of splat center
			10.0, 					// radius of splat
			sky_angle, 				// rotation in degrees
			white,					// change the color of splat
			&background, 			// image to be splatted upon
			&sky 					// image of the splat
			); 


// The "jaggie", a colored line with (somewhat) sharp angles
void generate_jaggie( fimage *result, fimage *splat )
{
	vfield f;		// Main vector field for iterating
	vect2 d, c, start, min, max;		// flow direction
	cluster k;
	int i;
	

	// set bounds of vector field larger than image
	min.x =  -1.0; min.y =  -1.0;
	max.x =  11.0; max.y =  11.0;
	vfield_initialize( 1000, 1000, min, max, &f );

	d.x = 1.0; d.y = 0.0;
	vfield_linear( d, &f );
	vfield_add_stripes( &f );
	vfield_normalize( &f );
	start.x = 0.0625; start.y = 4.0;
	cluster_initialize( &k );

	cluster_set_run( 	800, 			// Number of subjects in run
			start, 				// Starting point
			0.0625, 			// Initial step size
			1.0, 				// Step size proportional change per step
			0.0,				// No adjustment to angle
			&k );	

	cluster_set_bounds( f.min, f.max, &k );

	cluster_set_color( 	0.0, 		// Initial color
					0.005, 		// Color increment
					0.5, 		// Initial brightness
					1.0, 		// Brightness proportional change per step
					&k );

	cluster_set_size( true, 4.0, &k  );
	render_cluster( &f, &k, result, splat );
}

	/* or maybe not
	// One jag to rule them all
	c = ( rand() % 5 ) * 1.0 + 3.0;
	vv = 1.5 * rand1() - 0.75;
	printf(" one jag to rule them all c = %.2f  vv = %.2f\n", c, vv );
	v.y =  3.0 * vv / ( c - 2.0 );
	min.x = 2.0;
	max.x = c;
	f_box( min, max, v, f );
	v.y = -3.0 * vv / ( 8.0 - c );
	min.x = c;
	max.x = 8.0;
	f_box( min, max, v, f );
	*/

// Backed out of using constant values in function tree

typedef struct func_node func_node;

struct func_node {

	func_data value;	// Value of leaf or evaluated branch node
	func_type type;
	int nargs;
	gen_func fn;

	bool evaluated;		// Has this node been evaluated?
	bool choose;

	func_node *a;
	func_node *b;
	func_node *c;

	char name[256];

	bool use_constant_a, use_constant_b, use_constant_c;
	func_data constant_a, constant_b, constant_c;	// Optional constant values for parameters
	func_type constant_type_a, constant_type_b, constant_type_c;

};

func_data fnode_eval( func_node *fnode )
{
	func_data a, b, c;

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
				if( fnode->use_constant_a ) a = fnode->constant_a;
				else a = fnode_eval( fnode->a ); 
				fnode->value = fnode->fn.gf1( a ); 
				return fnode->value;
				}

			case 2: { 	
				if( fnode->use_constant_a ) a = fnode->constant_a;
					else a = fnode_eval( fnode->a );

					if( fnode->use_constant_b ) b = fnode->constant_b; 
					else b = fnode_eval( fnode->b );

					fnode->value = fnode->fn.gf2( a, b );
					return fnode->value;
				}

			case 3: {
				if( fnode->choose ) {					// To Do: Allow for constant values in choose
					if( fnode->use_constant_a ) a = fnode->constant_a;
					else a = fnode_eval( fnode->a ); 

					if( a.b ) {
						if( fnode->use_constant_b ) fnode->value = fnode->constant_b; 
						else fnode->value = fnode_eval( fnode->b );
						return fnode->value;
					}
					else {
						if( fnode->use_constant_c ) fnode->value = fnode->constant_c; 
						else fnode->value = fnode_eval( fnode->c );
						return fnode->value;
					}
				}
				else { 	
					if( fnode->use_constant_a ) a = fnode->constant_a; 
					else a = fnode_eval( fnode->a );

					if( fnode->use_constant_b ) b = fnode->constant_b; 
					else b = fnode_eval( fnode->b );

					if( fnode->use_constant_c ) c = fnode->constant_c; 
					else c = fnode_eval( fnode->c );

					fnode->value = fnode->fn.gf3( a, b, c ); 
					return fnode->value;
				}
			}
		}
	}
	return fnode->value;
}


char* fdata_string( func_data fn_data, func_type fn_type, char *str )
{
	switch( fn_type ) {
		case FN_BOOL  :	sprintf( str, "%s", bool_str( fn_data.b, str) );
		case FN_INT   :	sprintf( str, "%d", fn_data.i ); break;
		case FN_FLOAT :	sprintf( str, "%f", fn_data.f ); break;
		case FN_VECT2 :	sprintf( str, "[ %f, %f ]", fn_data.v.x, fn_data.v.y ); break;
		case FN_FRGB  :	sprintf( str, "[ %f, %f, %f ]", fn_data.c.r, fn_data.c.g, fn_data.c.b ); break;
   		default : sprintf( str, "");
   	}
   	return str;
}

func_type load_func_data( FILE *fp, func_data *fn_data, char *buffer, char *junk, bool *eof )
{
	file_get_string( fp, buffer, junk, eof );

	if( !strcmp( buffer, "bool") ) { 
		fn_data->b = file_get_bool( fp, buffer, junk, eof );
		return FN_BOOL;
	}
	else if( !strcmp( buffer, "int") ) {
		fn_data->i = file_get_int( fp, buffer, junk, eof );
		return FN_INT;
	}
	else if( !strcmp( buffer, "float") ) {	
		fn_data->f = file_get_float( fp, buffer, junk, eof );
		return FN_FLOAT;
	}
	else if( !strcmp( buffer, "vect2") ) { 	
		fn_data->v = file_get_vect2(  fp, buffer, junk, eof );
		return FN_VECT2;
	}
	else if( !strcmp( buffer, "frgb") ) {	
		fn_data->c = file_get_frgb(  fp, buffer, junk, eof );
		return FN_FRGB;
	}
	printf( "load_func_data - invalid type - buffer %s )\n", buffer );
	return FN_NULL;
}

void load_node( FILE *fp, func_node *fnode, func_tree *ftree, char *junk, bool *eof )
{
	char buffer[ 255 ];
	func_node *fn_a;
	func_node *fn_b;
	func_node *fn_c;
	gen_func gf;

	int n;
	func_type ftype;
	bool choose;
	func_type fn_type;
	func_data fn_data;

	fnode->use_constant_a = false;
	fnode->use_constant_b = false;
	fnode->use_constant_c = false;

	printf(" load_node: %s\n", fnode->name );
	file_get_string( fp, buffer, junk, eof );
	if( !( *eof ) ) {
		if( !strcmp( buffer, "leaf") ) { 
			fn_type = load_func_data( fp, &fn_data, buffer, junk, eof );
			fnode_init_leaf( fnode, fn_data, fn_type );
		}
		else {
			ftype = get_gen_func( buffer, &gf, &n, &choose );
			file_get_string( fp, buffer, junk, eof );
			if( !strcmp( buffer, "const") ) {
					fnode->constant_type_a = load_func_data( fp, &(fnode->constant_a), buffer, junk, eof );
					fnode->use_constant_a = true;
					fn_a = NULL;
			}
			else fn_a = ftree_index( ftree, buffer );

			if( n == 1 ) {
				fnode_init_1( fnode, gf.gf1, fn_a, ftype );
			}
			else {
				file_get_string( fp, buffer, junk, eof );
				if( !strcmp( buffer, "const") ) {
					fnode->constant_type_b = load_func_data( fp, &(fnode->constant_b), buffer, junk, eof );
					fnode->use_constant_b = true;
					fn_b = NULL;
				}
				else { fn_b = ftree_index( ftree, buffer ); }
				if( n==2 ) {
					fnode_init_2( fnode, gf.gf2, fn_a, fn_b, ftype );
				}
				else {	// n==3
					file_get_string( fp, buffer, junk, eof );
					if( !strcmp( buffer, "const") ) {
						fnode->constant_type_c = load_func_data( fp, &(fnode->constant_c), buffer, junk, eof );
						fnode->use_constant_c = true;
						fn_c = NULL;
					}
					else fn_c = ftree_index( ftree, buffer );
					fnode_init_3( fnode, gf.gf3, fn_a, fn_b, fn_c, ftype, choose );
				}
			}
		}
		fnode_print( fnode );
	}
}