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

// Initialize cluster with plausible default properties
void cluster_initialize( 	cluster *k, 
							const char *name,
							func_tree ftree;
							element *elem,
							int n )
{
	k->n = n;	// Default single subject because that's easier
	sprintf( k->name, "%s", name );
	ftree_copy( ftree, k->gen_func );
	k->elem = elem;

	// Bounding box
	k->bounded = false;					// Use bounding box? Iteration will stop if it leaves bounding box
	k->bmin.x = 0.0; k->bmin.y = 0.0;	
	k->bmax.x = 0.0; k->bmax.y = 0.0;

	// Color Filter
	k->color_filter = false;
	k->palette = NULL;

	k->n_elements_leaf = 		ftree_index( &(k->gen_func), "n_elements_leaf" );
	if( k->n_elements_leaf != NULL ) k->n_elements_leaf->leaf_val = n;

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

	func_node *branch_result;
	// Start with no branches -- future: add branches function
	k->branches->n = 0;			
}

void cluster_set_bounds( vect2 bmin, vect2 bmax, cluster *uck )
{
	uck->bounded = true;
	uck->bmin = bmin;
	uck->bmax = bmax;
}

bool cluster_set_leaf( cluster *uck, const char *leaf_name, func_data leaf_val )
{
	fnode *leaf = ftree_index( &(uck->gen_func), name );
	if( leaf == NULL ) return false;
	leaf->leaf_val = leaf_val;
	return true;
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
	vect2 w = uck->start;
	vect2 rwx;	// reflected vector in x
	vect2 rwy;	// reflected vector in y
	vect2 rwxy;	// reflected vector in x and y
	vect2 u;
	vect2 rmin, rmax;	// bounding box for reflected vector
	float step = uck->step_init;
	float ang = uck->ang_init;
	float rel_ang;
	int i=0;
	frgb tint;
	float color_index = uck->color_init;
	float size = uck->size_init;
	float brightness = uck->brightness;
	frgb  stencil_pixel;

	if( !uck->color_filter ) {
		tint.r = 1.0; tint.g = 1.0; tint.b = 1.0;
	}

	uck->time_leaf = fd_float( time );

	while( i < uck->n ) {

		uck->position_leaf->leaf_val 		= fd_vect2( w );
		uck->size_leaf->leaf_val     		= fd_float( size );
		uck->ang_leaf->leaf_val      		= fd_float( ang );
		uck->color_index_leaf->leaf_val     = fd_float( color_index );
		uck->brightness_leaf->leaf_val      = fd_float( brightness );
		uck->index_leaf->leaf_val			= fd_int( i );

		// Disable boundary test for now as splat could be large
		// if( in_bounds( w, uck->bmin, uck->bmax, size ) ) {	// Truncate cluster if out of bounds

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

		if( splat != NULL ) {
			// *** render here ***
			fimage_splat( 
				w, 				// coordinates of splat center
				size, 			// radius of splat
				-1.0 * ang, 	// rotation in degrees
				tint,			// change the color of splat
				target_fimg, 	// image to be splatted upon
				uck->splat 		// image of the splat
			);
		}
		

		// Iterate 

		w 			= fnode_eval( uck->position_result ).v;
		size 		= fnode_eval( uck->size_result ).f;
		ang 		= fnode_eval( uck->ang_result ).f;
		color_index	= fnode_eval( uck->color_index_result ).f;
		brightness 	= fnode_eval( uck->brightness_result ).f;
		
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

void cluster_list_copy( cluster_list *in, cluster_list *out )
{
	int i;
	cluster *clust_in = in->clusters;
	cluster *clust_out = out->clusters;

	if( in->n > 0 ) {
		for( i=0; i < in->n; i++ ) {
			cluster_copy( clust_in, clust_out );
			clust_in++;	
			clust_out++;
		}
	}
}

// ********************** Scene functions ********************** 

void scene_initialize( scene *scn )
{
	scn->use_mask = false;
	scn->use_stencil = false;
	scn->use_wave = false;
	scn->use_perturb = false;
	scn->perturb_steps = 0.0;
	scn->stencil_boost = 0.0;
	scn->reflect_x = false; scn->reflect_y = false;
	scn->x_mirror = 0; scn->y_mirror = 0;

	scn->base_fimg = NULL; scn->background_fimg = NULL; scn->mask_fimg = NULL; scn->splat_fimg = NULL; scn->stencil_fimg = NULL;
}

void scene_free( scene *scn )
{
	fimage_free( &( scn->base ) );
	fimage_free( &( scn->result ) );	
	fimage_free( &( scn->buffer ) );

	if( scn.use_mask ) {
		fimage_free( &( scn->overlay ) );
		fimage_free( &( scn->mask ) );
	}
	if( scn.use_stencil ) {
		fimage_free( &( scn->stencil ) );
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
	vect2 bound1 = v_set( -1.0, 1.0 );
	vect2 bound2 = v_set(  1.0, -1.0 );
	int i;

	fp = fopen( filename, "r" );
	while( !eof ) { 			
		file_get_string( fp, buffer, junk, &eof );
		if( !eof ) {
			if( !strcmp( buffer, "name") ) 				file_get_string( fp, scn->name, junk, &eof );

			if( !strcmp( buffer, "base_file") ) 		file_get_string( fp, scn->base_file, junk, &eof );
			if( !strcmp( buffer, "use_mask") ) 			scn->use_mask = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "overlay_file") ) 		file_get_string( fp, scn->overlay_file, junk, &eof );
			if( !strcmp( buffer, "mask_file") ) 		file_get_string( fp, scn->mask_file, junk, &eof );
			//if( !strcmp( buffer, "splat_file") ) 		file_get_string( fp, scn->splat_file, junk, &eof );
			if( !strcmp( buffer, "use_stencil") ) 		scn->use_stencil = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "stencil_boost") ) 	scn->stencil_boost = file_get_float( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "stencil_file") ) 		file_get_string( fp, scn->stencil_file, junk, &eof );


			if( !strcmp( buffer, "bounds") ) 			{ 	scn->bound1 = file_get_vect2( fp, buffer, junk, &eof );
															scn->bound2 = file_get_vect2( fp, buffer, junk, &eof ); }

			if( !strcmp( buffer, "reflect_y") ) 		scn->reflect_y = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "reflect_y_line") ) 	scn->reflect_y_line = file_get_int( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "output_basename") ) 	file_get_string( fp, scn->output_basename, junk, &eof );
			if( !strcmp( buffer, "use_wave") ) 			scn->use_wave = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "use_perturb") ) 		scn->use_perturb = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "perturb_steps") ) 	scn->perturb_steps = file_get_int( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "debug_me") ) 			debug_me = file_get_bool( fp, buffer, junk, &eof );

			// load elements (splat files)

			int n;
			char *name;
			fimage *splat;

			if( !strcmp( buffer, "splats") ) 			{	
				n = file_get_int( fp, buffer, junk, &eof );
				scn->scene_elems.n = n;
				scn->scene_elems.elems = (element *)malloc( n * sizeof( element ) );
				for( i=0; i<n, i++ ) {
					name = scn->scene_elems.elems[i].name;
					splat = &( scene_elems.elems[i].splat );
					file_get_string( fp, name, junk, &eof );
					fimage_load( name, splat ); 
				}
			}

			// load functions
			if( !strcmp( buffer, "functions") ) 		{	
				n = file_get_int( fp, buffer, junk, &eof );
				scn->scene_funcs.n = n;
				scn->scene_funcs.trees = (func_tree *)malloc( n * sizeof( func_tree ) );
				for( i=0; i<n; i++ ) {
					file_get_string( fp, scn->scene_funcs.trees[i].name, junk, &eof );
					ftree_load( &( scn->scene_funcs.trees[i] ), scn->scene_funcs.trees[i].name );
				}
			}

			// load clusters (contained within scene file)
		}
	}
	fclose( fp );

	fimage *base = &( scn->base_fimg );
	fimage *result = &( scn->result_fimg );
	fimage *overlay = &( scn->overlay_fimg );
	fimage *mask = &( scn->mask_fimg );
	fimage *stencil = &( scn->stencil_fimg );

	// load image files
	load_err = fimage_load( scn->base_file, base );					if( !load_err ) return 0;
	fimage_set_bounds( bound1, bound2, base );

	fimage_init( base->xdim,  base->ydim, result );	
	fimage_set_bounds( bound1, bound2, result );
	

	if( scn.use_mask ) {
		// load background image, duplicate to background buffer
		load_err = fimage_load( scn->overlay_file, overlay );		if( !load_err ) return 0;
		fimage_set_bounds( bound1, bound2, overlay );

		load_err = fimage_load( scn->mask_file, mask );				if( !load_err ) return 0;
		fimage_set_bounds( bound1, bound2, mask );
	}

	if( scn.use_stencil ) {
		load_err = fimage_load( scn.stencil_file, stencil );			if( !load_err ) return 0;
		fimage_set_bounds( bound1, bound2, stencil );
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

	fimage_copy_contents( base, result );

	// render top-level clusters
	for( i = 0, i < scn->clusters.n, i++ ) {
		if( scn->clusters.cluster[ i ].tlc ) cluster_render( scn->clusters.cluster[ i ], result, time ); // automatically render subclusters
	}

	// Clip colors in image to prevent excessive darkening
	fimage_clip( 0.0, 1.0, &( result ) );

	// may need to modify these fimg functions
	if( scn->reflect_y ) fimage_reflect_y( scn->y_mirror, true, render_to, result);	// reflect image if required
	if( scn.use_mask ) fimage_apply_mask( overlay, mask, result );

}


