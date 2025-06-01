#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "frgb.h"
#include "vect2.h"
#include "joy_io.h"
#include "func_node.h"
#include "fimage.h"
#include "scene.h"


// ********************** Element functions ********************** 

// Sets options to false, sets images to stubs
void element_init( element *elem, const char *name )
{
	sprintf( elem->name, "%s", name );
	fimage_stub( &( elem->splat ) );
	elem->use_mask = false;
	fimage_stub( &( elem->mask ) );
	elem->use_warp = false;
	elem->warp.n = 0;
	elem->warp.nodes = NULL;
}

bool element_load( element *elem, FILE *fp, char *junk )	// returns end of file condition
{
	char buffer[256];
	bool done = false;
	bool eof = false;

	while( ( !done ) && ( !eof ) ) {
		file_get_string( fp, buffer, junk, &eof );
		if( !eof ) {
			if( !strcmp( buffer, "name") ) 				file_get_string( fp, elem->name, junk, &eof );
			if( !strcmp( buffer, "splat") ) 		{	file_get_string( fp, buffer, junk, &eof );		fimage_load( buffer, &( elem->splat ) );	}
			if( !strcmp( buffer, "use_mask") ) 			elem->use_mask = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "mask") ) 			{	file_get_string( fp, buffer, junk, &eof );		fimage_load( buffer, &( elem->mask ) );	}
			if( !strcmp( buffer, "use_warp") ) 			elem->use_mask = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "warp") ) 			{	file_get_string( fp, buffer, junk, &eof );		ftree_load( &( elem->warp ), buffer );	}
			if( !strcmp( buffer, "break") )				done = true;
		}
		else return eof;
	}
	return eof;
}

// ********************** Element list functions ********************** 

void element_list_init( element_list *elist, int n, const char *name )
{
	elist->elems = (element *)malloc( n * sizeof( element ) );
	sprintf( elist->name, "%s", name );
}

element *element_list_index( element_list *elist, const char *name )
{
	int i;
	element *elem = elist->elems;

	for( i = 0; i < elist->n; i++ )
	{
		if( !strcmp( name, elem->name) ) return elem;
		elem++;
	}
	return NULL;	// name not found
}

// ********************** Cluster functions ********************** 

// Initialize cluster with plausible default properties
void cluster_init( cluster *k )
{
	k->n = 1;	// Default single subject because that's easier
	sprintf( k->name, "" );

	k->elem = 					NULL;

	// Bounding box
	k->bounded = 				false;		// Use bounding box? Iteration will stop if it leaves bounding box
	k->bound_min.x = -1.0; k->bound_min.y = 1.0;	
	k->bound_max.x =  1.0; k->bound_max.y = -1.0;

	// Color Filter
	k->color_filter = 			false;
	k->color_palette = 			NULL;

	k->n_elements_leaf = 		NULL;

	k->index_leaf = 			NULL;		// keeps track of current iteration of cluster

	k->time_leaf = 				NULL;

	k->origin_leaf = 			NULL;		
	k->position_leaf = 			NULL;		// used recursively to generate
	k->position_result = 		NULL;	

	k->size_init_leaf = 		NULL;
	k->size_leaf = 				NULL;		// used recursively to generate
	k->size_result = 			NULL;

	k->ang_init_leaf = 			NULL;
	k->ang_leaf = 				NULL;		// used recursively to generate
	k->ang_result = 			NULL;

	// for now use palette and brightness (could get more sophisticated with color calculation)
	k->color_index_init_leaf = 	NULL;
	k->color_index_leaf = 		NULL;
	k->color_index_result = 	NULL;

	k->brightness_init_leaf = 	NULL;
	k->brightness_leaf = 		NULL;
	k->brightness_result = 		NULL;

	k->branch_result = 			NULL;
	// Start with no branches -- future: add branches function
	k->branches.n = 0;			
}

void cluster_copy( cluster *in, cluster *out )
{
	// TBI 
}

void cluster_set_gen_func( cluster *k, func_tree *gen_func )
{
	printf( "\ncluster_set_gen_func called \n\n" );
	ftree_copy( gen_func, &( k->gen_func ) );

	k->n_elements_leaf = 		ftree_index( &(k->gen_func), "n_elements_leaf" );
	//if( k->n_elements_leaf != NULL ) k->n_elements_leaf->value = fd_int( k->n );
	if( k->n_elements_leaf != NULL ) k->n = k->n_elements_leaf->value.i;

	k->index_leaf = 			ftree_index( &(k->gen_func), "index_leaf" );		// keeps track of current iteration of cluster

	k->time_leaf = 				ftree_index( &(k->gen_func), "time_leaf" );

	k->origin_leaf = 			ftree_index( &(k->gen_func), "origin_leaf" );		
	k->position_leaf = 			ftree_index( &(k->gen_func), "position_leaf" );		// used recursively to generate
	k->position_result = 		ftree_index( &(k->gen_func), "position_result" );	

	k->size_init_leaf = 		ftree_index( &(k->gen_func), "size_init_leaf" );
	k->size_leaf = 				ftree_index( &(k->gen_func), "size_leaf" );			// used recursively to generate
	k->size_result = 			ftree_index( &(k->gen_func), "size_result" );

	k->ang_init_leaf = 			ftree_index( &(k->gen_func), "ang_init_leaf" );
	k->ang_leaf = 				ftree_index( &(k->gen_func), "ang_leaf" );			// used recursively to generate
	k->ang_result = 			ftree_index( &(k->gen_func), "ang_result" );

	// for now use palette and brightness (could get more sophisticated with color calculation)
	k->color_index_init_leaf = 	ftree_index( &(k->gen_func), "color_index_init_leaf" );
	k->color_index_leaf = 		ftree_index( &(k->gen_func), "color_index_leaf" );
	k->color_index_result = 	ftree_index( &(k->gen_func), "color_index_result" );

	k->brightness_init_leaf = 	ftree_index( &(k->gen_func), "brightness_init_leaf" );
	k->brightness_leaf = 		ftree_index( &(k->gen_func), "brightness_leaf" );
	k->brightness_result = 		ftree_index( &(k->gen_func), "brightness_result" );

	k->branch_result = 			ftree_index( &(k->gen_func), "branch_result" );
} 

void cluster_set_bounds( cluster *uck, vect2 bound_min, vect2 bound_max)
{
	uck->bounded = true;
	uck->bound_min = bound_min;
	uck->bound_max = bound_max;
}

void cluster_set_n( cluster *k, int n )
{
	k->n = n;	// Default single subject because that's easier
	if( k->n_elements_leaf != NULL ) k->n_elements_leaf->value = fd_int( k->n );
}

void cluster_set_name( cluster *k, const char *name )
{
		sprintf( k->name, "%s", name );
}

void cluster_set_element( cluster *k, element *elem )
{
	k->elem = elem;
	// future - connect element warp parameters to gen_func results
}

bool cluster_set_leaf( cluster *uck, const char *leaf_name, func_data value )
{
	func_node *leaf = ftree_index( &(uck->gen_func), leaf_name );
	if( leaf == NULL ) return false;
	leaf->value = value;
	return true;
}

void cluster_load_branches( cluster *k, scene *scn, FILE *fp, char *str, char *junk, bool *eof )
{
	// TBI near future

	// Copy branch clusters

	// Copy branch functions

	// Connect cluster generative function results to branch function inputs
}

bool cluster_load( cluster *k, scene *scn, FILE *fp, char *junk ) // returns end of file condition
{
	char buffer[ 256 ];
	bool done = false;
	bool eof = false;
	func_tree *gen_func;
	element *elem;
	cluster_init( k );
	cluster_set_bounds( k, scn->bound_min, scn->bound_max );

	//printf( " \n cluster_load called eof = %s, done = %s\n\n", formatBool( eof ), formatBool( done ) );

	while( !done ) {
		file_get_string( fp, buffer, junk, &eof );
		if( !eof ) {
			//printf( " \n cluster_load - string comparison \n\n" );
			if( !strcmp( buffer, "name") ) 				file_get_string( fp, k->name, junk, &eof );
			if( !strcmp( buffer, "top_level") ) 		k->tlc = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "gen_func") ) 		{	
														file_get_string( fp, buffer, junk, &eof );	
														gen_func = ftree_list_index( &( scn->scene_funcs), buffer );
														cluster_set_gen_func( k, gen_func );	
													}
			if( !strcmp( buffer, "element") ) 		{	
														file_get_string( fp, buffer, junk, &eof );	
														elem = element_list_index( &( scn->scene_elems ), buffer );	
														cluster_set_element( k, elem );
													}
			if( !strcmp( buffer, "branches") ) 		{	
														file_get_string( fp, buffer, junk, &eof );	
														cluster_load_branches( k, scn, fp, buffer, junk, &eof );
													}
			if( !strcmp( buffer, "break") )			done = true;
		}
		else return eof;
	}
	return eof;
}

// set branch / branches function here

/* void cluster_set_run( int n, vect2 origin, float step, float prop, float ang_offset, cluster *uck )
{
	uck->n = n;
	uck->start = start;
	uck->step = step;
	uck->prop = prop;
	uck->ang_offset = ang_offset;
}


void cluster_set_color( palette *color_palette, float start_color, float color_inc, float brightness, float brightness_prop, cluster *uck )
{
	uck->color_palette		= color_palette;
	uck->color_filter 		= true;
	uck->start_color 		= start_color;
	uck->color_inc 			= color_inc;
	uck->brightness 		= brightness;
	uck->brightness_prop 	= brightness_prop;
}

void cluster_set_size( bool size_prop, float size, cluster *uck )
{
	uck->size_prop = size_prop;
	uck->size = size;
}
*/

void cluster_render( cluster *uck, fimage *target_fimg, float time )
{
	vect2 position;
	vect2 rwx;	// reflected vector in x
	vect2 rwy;	// reflected vector in y
	vect2 rwxy;	// reflected vector in x and y
	vect2 rmin, rmax;	// bounding box for reflected vector
	float ang, rel_ang, color_index, size, brightness;
	frgb  stencil_pixel, tint;
	int i=0;

	printf( "cluster_render called\n" );


	if( !uck->color_filter ) {
		tint.r = 1.0; tint.g = 1.0; tint.b = 1.0;
	}

	uck->time_leaf->value 			= fd_float( time );

	// Initialize iterative elements
	uck->position_leaf->value 		= uck->origin_leaf->value;
	uck->size_leaf->value     		= uck->size_init_leaf->value;
	uck->ang_leaf->value      		= uck->ang_init_leaf->value;
	uck->color_index_leaf->value    = uck->color_index_init_leaf->value;
	uck->brightness_leaf->value     = uck->brightness_init_leaf->value;

	while( i < uck->n ) {
		printf( "cluster_render i=%d\n", i );
		uck->index_leaf->value = fd_int( i );

		ftree_reset( &( uck->gen_func ) );
		position 	= fnode_eval( uck->position_result ).v;
		size 		= fnode_eval( uck->size_result ).f;
		ang 		= fnode_eval( uck->ang_result ).f;
		color_index	= fnode_eval( uck->color_index_result ).f;
		brightness 	= fnode_eval( uck->brightness_result ).f;

		// Disable boundary test for now as splat could be large
		// Future: test for boundary based on bounds padded by size of splat (careful of negative scale condition)
		// if( in_bounds( w, uck->bound_min, uck->bound_max, size ) ) {	// Truncate cluster if out of bounds

		// build color function into tree
		if( uck->color_filter ) {
			tint = palette_index( color_index, uck->color_palette );
			tint.r *= brightness;
			tint.g *= brightness;
			tint.b *= brightness;
		}
		else {
			tint.r = brightness;
			tint.g = brightness;
			tint.b = brightness;
		}

		/* defer stencil implementation
		if( scn->use_stencil ) {
			stencil_pixel = fimage_sample( w, false, scn->stencil_fimg );
			tint.r *=  ( 0.25 + stencil_pixel.r * scn->stencil_boost );
			tint.g *=  ( 0.25 + stencil_pixel.g * scn->stencil_boost );
			tint.b *=  ( 0.25 + stencil_pixel.b * scn->stencil_boost );
		}
		*/

		// *** render here ***
		fimage_splat( 
			position, 					// coordinates of splat center
			size, 						// radius of splat
			-1.0 * ang, 				// rotation in degrees
			tint,						// change the color of splat
			target_fimg, 				// image to be splatted upon
			&( uck->elem->splat )		// image of the splat
		);

		// iterate
		uck->position_leaf->value 		= fd_vect2( position );
		uck->size_leaf->value     		= fd_float( size );
		uck->ang_leaf->value      		= fd_float( ang );
		uck->color_index_leaf->value    = fd_float( color_index );
		uck->brightness_leaf->value     = fd_float( brightness );
		i++;
	}
}

// ********************** Cluster List functions ********************** 

void cluster_list_init( cluster_list *clist, int n, const char *name)
{
	clist->n = n;
	sprintf( clist->name, "%s", name );
	if( n > 0 ) clist->clusters = ( cluster * )malloc( n * sizeof( cluster ) );
	else clist->clusters = NULL;
}

cluster *cluster_list_index( cluster_list *clist, const char *name )
{
	int i;
	cluster *uck = clist->clusters;

	for( i = 0; i < clist->n; i++ )
	{
		if( !strcmp( name, uck->name) ) return uck;
		uck++;
	}
	return NULL;	// name not found
}

void cluster_list_copy( cluster_list *in, cluster_list *out )
{
	int i;
	cluster *clust_in = in->clusters;
	cluster *clust_out = out->clusters;

	if( in->n > 0 ) {
		for( i = 0; i < in->n; i++ ) {
			cluster_copy( clust_in, clust_out );
			clust_in++;	
			clust_out++;
		}
	}
}

// ********************** Scene functions ********************** 

void scene_init( scene *scn )
{
	scn->use_mask = false;
	scn->use_stencil = false;
	scn->use_wave = false;
	scn->use_perturb = false;
	scn->perturb_steps = 0.0;
	scn->stencil_boost = 0.0;
	scn->reflect_x = false; scn->reflect_y = false;
	scn->x_mirror = 0; scn->y_mirror = 0;

	fimage_stub( &scn->base_fimg ); 
	fimage_stub( &scn->overlay_fimg ); 
	fimage_stub( &scn->mask_fimg ); 
	fimage_stub( &scn->result_fimg ); 
	fimage_stub( &scn->stencil_fimg );

	scn->bound_min = v_set( -1.0,  1.0 );
	scn->bound_max = v_set(  1.0, -1.0 );
}

void scene_free( scene *scn )
{
	fimage_free( &( scn->base_fimg ) );
	fimage_free( &( scn->result_fimg ) );	
	if( scn->use_mask ) {
		fimage_free( &( scn->overlay_fimg ) );
		fimage_free( &( scn->mask_fimg ) );
	}
	if( scn->use_stencil ) {
		fimage_free( &( scn->stencil_fimg ) );
	}

	// Free each element

	// Free each function
}

// Reads a .json scene file
// Not designed to be bulletproof
// Returns "true" if error in load
int scene_load( scene *scn, const char *filename )
{
	FILE *fp;
	bool eof = false;		// end of file or other error detected
	char buffer[255];
	char junk[1024];
	int i;

	int n;
	fimage *splat;
	fimage *mask;
	element_list *elist = &(scn->scene_elems);
	char name[ 256 ];
	element *elem_ptr;
	cluster *k;
	bool load_err;

	fp = fopen( filename, "r" );
	while( !eof ) { 			
		file_get_string( fp, buffer, junk, &eof );
		if( !eof ) {
			if( !strcmp( buffer, "name") ) 				file_get_string( fp, scn->name, junk, &eof );

			if( !strcmp( buffer, "base_file") ) 		file_get_string( fp, scn->base_file, junk, &eof );
			if( !strcmp( buffer, "use_mask") ) 			scn->use_mask = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "overlay_file") ) 		file_get_string( fp, scn->overlay_file, junk, &eof );
			if( !strcmp( buffer, "mask_file") ) 		file_get_string( fp, scn->mask_file, junk, &eof );
			if( !strcmp( buffer, "use_stencil") ) 		scn->use_stencil = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "stencil_boost") ) 	scn->stencil_boost = file_get_float( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "stencil_file") ) 		file_get_string( fp, scn->stencil_file, junk, &eof );


			if( !strcmp( buffer, "bounds") ) 			{ 	scn->bound_min = file_get_vect2( fp, buffer, junk, &eof );
															scn->bound_max = file_get_vect2( fp, buffer, junk, &eof ); }

			if( !strcmp( buffer, "reflect_y") ) 		scn->reflect_y = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "y_mirror") ) 			scn->y_mirror = file_get_int( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "output_basename") ) 	file_get_string( fp, scn->output_basename, junk, &eof );
			if( !strcmp( buffer, "use_wave") ) 			scn->use_wave = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "use_perturb") ) 		scn->use_perturb = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "perturb_steps") ) 	scn->perturb_steps = file_get_int( fp, buffer, junk, &eof );
			//if( !strcmp( buffer, "debug_me") ) 		debug_me = file_get_bool( fp, buffer, junk, &eof );

			// load elements 
			if( !strcmp( buffer, "elements") ) 			{	
				n = file_get_int( fp, buffer, junk, &eof );
				sprintf( name, "%s elements", scn->name );
				element_list_init( elist, n, name );
				elem_ptr = elist->elems;
				for( i = 0; i < n; i++ ) {
					eof = element_load( elem_ptr, fp, junk );
					elem_ptr++;
				}
			}

			// load functions
			if( !strcmp( buffer, "functions") ) 		{	
				n = file_get_int( fp, buffer, junk, &eof );
				ftree_list_init( &( scn->scene_funcs ), n );
				sprintf( name, "%s_generative_functions", scn->name );
				ftree_list_set_name( &( scn->scene_funcs ), name );
				for( i=0; i<n; i++ ) {
					file_get_string( fp, scn->scene_funcs.trees[i].name, junk, &eof );
					ftree_load( &( scn->scene_funcs.trees[i] ), scn->scene_funcs.trees[i].name );
				}
			}

			// load clusters (contained within scene file)
			if( !strcmp( buffer, "clusters") ) 		{	
				n = file_get_int( fp, buffer, junk, &eof );
				sprintf( name, "%s_clusters", scn->name );
				cluster_list_init( &(scn->clusters), n, name );
				k = scn->clusters.clusters;
				for( i=0; i<n; i++ ) {
					eof = cluster_load( k, scn, fp, junk );
					printf(" \n cluster_load complete\n\n");
					k++;
				}
			}
		}
	}
	fclose( fp );

	fimage *base = &( scn->base_fimg );
	fimage *result = &( scn->result_fimg );
	fimage *overlay = &( scn->overlay_fimg );
	fimage *mask_fimg = &( scn->mask_fimg );
	fimage *stencil = &( scn->stencil_fimg );

	// load image files
	load_err = fimage_load( scn->base_file, base );					if( !load_err ) return 0;
	fimage_set_bounds( scn->bound_min, scn->bound_max, base );

	fimage_init( base->xdim,  base->ydim, result );	
	fimage_set_bounds( scn->bound_min, scn->bound_max, result );
	

	if( scn->use_mask ) {
		// load background image, duplicate to background buffer
		load_err = fimage_load( scn->overlay_file, overlay );		if( !load_err ) return 0;
		fimage_set_bounds( scn->bound_min, scn->bound_max, overlay );

		load_err = fimage_load( scn->mask_file, mask );				if( !load_err ) return 0;
		fimage_set_bounds( scn->bound_min, scn->bound_max, mask_fimg );
	}

	if( scn->use_stencil ) {
		load_err = fimage_load( scn->stencil_file, stencil );			if( !load_err ) return 0;
		fimage_set_bounds( scn->bound_min, scn->bound_max, stencil );
	}

	// load cluster files

	return 1;
}

void scene_render( float time, scene *scn )
{
	int i;
	fimage *base = &( scn->base_fimg );
	fimage *result = &( scn->result_fimg );
	fimage *overlay = &( scn->overlay_fimg );
	fimage *mask = &( scn->mask_fimg );
	fimage *stencil = &( scn->stencil_fimg );

	printf( "scene_render called\n" );

	fimage_copy_contents( base, result );

	// render top-level clusters
	for( i = 0; i < scn->clusters.n; i++ ) {
		printf( "scene_render - cluster %d\n", i );
		if( scn->clusters.clusters[ i ].tlc ) printf( "scene_render - tlc true\n ");
		else printf( "scene_render - tlc false\n ");
		if( scn->clusters.clusters[ i ].tlc ) cluster_render( &( scn->clusters.clusters[ i ] ), result, time ); // automatically render subclusters
	}

	// Clip colors in image to prevent excessive darkening
	fimage_clip( 0.0, 1.0, result );

	// may need to modify these fimg functions
	if( scn->reflect_y ) fimage_reflect_y( scn->y_mirror, true, result);	// reflect image if required
	if( scn->use_mask ) fimage_apply_mask( overlay, mask, result );

}


