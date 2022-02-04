#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "frgb.h"
#include "vect2.h"
#include "fimage.h"
#include "scene.h"

// Initialize cluster with plausible default properties
void cluster_initialize( cluster *k )
{
	k->n = 1;	// Default single subject because that's easier
	k->start.x = 0.0; k->start.y = 0.0;	// Origin

	// Size properties
	k->size = 1.0;
	k->size_prop = true;

	// Step properties
	k->step = 1.0;
	k->prop = 1.0;
	k->ang_offset = 0.0;	// Move directly on vector lines

	// Bounding box
	k->bounded = false;					// Use bounding box? Iteration will stop if it leaves bounding box
	k->bmin.x = 0.0; k->bmin.y = 0.0;	
	k->bmax.x = 0.0; k->bmax.y = 0.0;

	// Element orientation
	k->ang_init = 0.0;
	k->ang_inc = 0.0;
	k->ang_relative = true;	// We like the relative angle

	// Color Filter
	k->color_filter = false;
	k->brightness_ramp = false;
	k->brightness_ramp_length = 1;
	k->start_color = 0.0;
	k->color_inc = 0.0;
	k->brightness = 1.0;
	k->brightness_prop = 1.0;

	k->subclusters = NULL;			// Start with no subclusters
}

void cluster_set_run( int n, vect2 start, float step, float prop, float ang_offset, cluster *uck )
{
	uck->n = n;
	uck->start = start;
	uck->step = step;
	uck->prop = prop;
	uck->ang_offset = ang_offset;
}

void cluster_set_bounds( vect2 bmin, vect2 bmax, cluster *uck )
{
	uck->bounded = true;
	uck->bmin = bmin;
	uck->bmax = bmax;
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

void scene_initialize( scene *scn )
{
	scn->use_mask = false;
	scn->use_stencil = false;
	scn->use_wave = false;
	scn->use_perturb = false;
	scn->perturb_steps = 0.0;
	scn->stencil_boost = 0.0;
	scn->reflect_x = false; scn->reflect_y = false;
	scn->reflect_x_line = 0; scn->reflect_y_line = 0;

	scn->base_fimg = NULL; scn->background_fimg = NULL; scn->mask_fimg = NULL; scn->splat_fimg = NULL; scn->stencil_fimg = NULL;
}

// next string in file
void file_get_string( FILE *fp, char *str, char *junk, bool *eof )
{
	int nargs;

	// must have junk - otherwise the string result goes into junk
	nargs = fscanf(fp, "%[^\"]\"%[^\"]\"", junk, str);
	if( nargs != 2 ) *eof = true;
	else eof = false;
	printf( "file_get_string: %s\n",str);
}

bool file_get_bool( FILE *fp, char *str, char *junk, bool *eof )
{
	int nargs;

	// must have junk - otherwise the string result goes into junk
	nargs = fscanf(fp, "%[^a-z]%[a-z]", junk, str);
	if( nargs != 2 ) *eof = true;
	else eof = false;
	printf( "file_get_bool: %s\n",str);
	return( !strcmp( str, "true") );
}

int file_get_int( FILE *fp, char *str, char *junk, bool *eof )
{
	int result;
	int nargs;

	nargs = fscanf(fp, "%[^0-9,^-]%[0-9,-]", junk, str);
	if( nargs != 2 ) *eof = true;
	else eof = false;
	result = atoi( str );
	printf( "file_get_int: %d\n",result);
	return( result );
}

float file_get_float( FILE *fp, char *str, char *junk, bool *eof )
{
	float result;
	int nargs;

	nargs = fscanf(fp, "%[^0-9,^-]%[0-9,-,+,.,E,e]", junk, str);
	if( nargs != 2 ) *eof = true;
	else eof = false;
	result = ( float )atof( str );
	printf( "file_get_float: %f\n",result);
	return( result );
}

// Reads a .json scene file
// Not designed to be bulletproof
void scene_load( scene *scn, const char *filename )
{
	FILE *fp;
	bool eof = false;		// end of file or other error detected
	char buffer[255];
	char junk[1024];

	fp = fopen( filename, "r" );
	while( !eof ) { 			
		file_get_string( fp, buffer, junk, &eof );
		if( !eof ) {
			if( !strcmp( buffer, "base_file") ) 		file_get_string( fp, scn->base_file, junk, &eof );
			if( !strcmp( buffer, "use_mask") ) 			scn->use_mask = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "background_file") ) 	file_get_string( fp, scn->background_file, junk, &eof );
			if( !strcmp( buffer, "mask_file") ) 		file_get_string( fp, scn->mask_file, junk, &eof );
			if( !strcmp( buffer, "splat_file") ) 		file_get_string( fp, scn->splat_file, junk, &eof );
			if( !strcmp( buffer, "use_stencil") ) 		scn->use_stencil = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "stencil_boost") ) 	scn->stencil_boost = file_get_float( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "stencil_file") ) 		file_get_string( fp, scn->stencil_file, junk, &eof );		
			if( !strcmp( buffer, "reflect_y") ) 		scn->reflect_y = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "reflect_y_line") ) 	scn->reflect_y_line = file_get_int( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "output_basename") ) 	file_get_string( fp, scn->output_basename, junk, &eof );
			if( !strcmp( buffer, "use_wave") ) 			scn->use_wave = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "use_perturb") ) 		scn->use_perturb = file_get_bool( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "perturb_steps") ) 	scn->perturb_steps = file_get_int( fp, buffer, junk, &eof );
			if( !strcmp( buffer, "debug_me") ) 			debug_me = file_get_bool( fp, buffer, junk, &eof );


		}
	}
	fclose( fp );
}


// "runs" are clusters of subjects advected through vector field
// run can continue a certain number of iterations or until crossing boundary condition
// this function can adjust number of elements in run if it wants to
// occasional mutation changes single member of run or remainder of run
void render_cluster( vfield *vf, cluster *uck, fimage *result, fimage *splat, scene *scn )
{
	vect2 w = uck->start;
	vect2 rwx;	// reflected vector in x
	vect2 rwy;	// reflected vector in y
	vect2 rwxy;	// reflected vector in x and y
	vect2 u;
	vect2 rmin, rmax;	// bounding box for reflected vector
	float step = uck->step;
	float ang = uck->ang_init;
	float rel_ang;
	int i=0;
	frgb tint;
	float color_index = uck->start_color;
	float size = uck->size;
	float brightness = uck->brightness;
	frgb  stencil_pixel;

	if( !uck->color_filter ) {
		tint.r = 1.0; tint.g = 1.0; tint.b = 1.0;
	}

	while( i < uck->n ) {

		//calculate angle
		rel_ang = ang;

		if( uck->size_prop ) size = step * uck->size;

		if( uck->ang_relative )
		{
			u = vfield_smooth_index( w, vf );
			v_rotate( u, uck->ang_offset );
			u.x *= step; u.y *= step;
			rel_ang = add_angle( rel_ang, vtoa( u ) );
		}

		if( in_bounds( w, uck->bmin, uck->bmax, size ) ) {	// Truncate cluster if out of bounds

			if( uck->color_filter ) {
				tint = palette_index( color_index, uck->color_palette );
				tint.r *= brightness;
				tint.g *= brightness;
				tint.b *= brightness;
				brightness *= uck->brightness_prop;
			}
			else {
				tint.r = brightness;
				tint.g = brightness;
				tint.b = brightness;
				brightness *= uck->brightness_prop;
			}

			if( uck->brightness_ramp ) {
				if( i < uck->brightness_ramp_length ) {
					tint.r *= 1.0 * (i + 1) / uck->brightness_ramp_length;
					tint.g *= 1.0 * (i + 1) / uck->brightness_ramp_length;
					tint.b *= 1.0 * (i + 1) / uck->brightness_ramp_length;
				}
				if( i > uck->n - uck->brightness_ramp_length ) {
					tint.r *= 1.0 * (uck->n - i) / uck->brightness_ramp_length;
					tint.g *= 1.0 * (uck->n - i) / uck->brightness_ramp_length;
					tint.b *= 1.0 * (uck->n - i) / uck->brightness_ramp_length;
				}

			}

			if( scn->use_stencil ) {
				stencil_pixel = fimage_sample( w, false, scn->stencil_fimg );
				tint.r *=  ( 0.25 + stencil_pixel.r * scn->stencil_boost );
				tint.g *=  ( 0.25 + stencil_pixel.g * scn->stencil_boost );
				tint.b *=  ( 0.25 + stencil_pixel.b * scn->stencil_boost );
			}

			// *** render here ***
			fimage_splat( 
				w, 				// coordinates of splat center
				size, 			// radius of splat
				-1.0 * rel_ang, // rotation in degrees
				tint,			// change the color of splat
				result, 		// image to be splatted upon
				splat 			// image of the splat
			);
		}

		// Iterate 
		color_index += uck->color_inc;
		w = vfield_advect( w, vf, step, uck->ang_offset );	// move through vector field 
		if( !in_bounds( w, vf->min, vf->max, 0.0 ) ) i=uck->n;	// stop iteration if vector goes out of bounds

		ang = add_angle( ang, uck->ang_inc );
		step *= uck->prop;

		// blend through property (size color etc)
		
		i++;
	}
}

void render_cluster_list( int nruns, float ang_offset, cluster *clist, vfield *vf, fimage *splat, fimage *result, scene *scn )
{
	cluster *uck = clist;
	int run;

	if( debug_me ) printf("render_cluster_list\n");

	for(run = 0; run < nruns; run++ ) {
		if( run == 3 ) critical_run = true;
		uck->ang_offset = ang_offset;
		render_cluster( vf, uck, result, splat, scn );
		uck++;
	}
}

// creates a grouping of runs evenly spaced in a circle
// occasional mutation changing some aspect of a run
// needs to be fixed
void generate_circle( 	vect2 c, 	// center of circle
						float r, 	// radius of circle
						float start_ang,	// starting angle in degrees
						int m,			// number of runs around circle
						cluster *uck,
						vfield *f )
{
	int i;
	float theta = TAU * start_ang / 360.0;

	for(i=0; i<m; i++)
	{
		uck->start.x = c.x + r * cos(theta);
		uck->start.y = c.y + r * sin(theta);
		//generate_cluster( f, uck, );
		theta += TAU / m;
	}
}

// generates a cluster in a regular grid, aligned along vector field
void generate_grid( vect2 min, vect2 max, int xsteps, int ysteps, float mutation_rate, vfield *f )
{
	int i, j;
	vect2 v,w;

	for( i=0; i<xsteps; i++ ) 
	{
		for( j=0; j<ysteps; j++ )
		{
			v.x = min.x + i * ( max.x - min.x ) / ( xsteps - 1 );
			v.y = min.y + j * ( max.y - min.y ) / ( ysteps - 1 );
			printf("subj %d %d \n", i, j);
			if( rand1() < mutation_rate ) printf("MUTANT\n");
			printf("  x = %.2f\n", v.x );
			printf("  y = %.2f\n", v.y );
			w = vfield_smooth_index( v, f );
			printf("  w.x = %.2f  w.y = %.2f\n",w.x,w.y);
			printf("  angle = %.0f\n\n", vtoa( w ) );
		}
	}
}

// creates a grouping of runs in a line
// generate_line() (stub)

// creates a random distribution of runs
// future: create a structure with the random parameters
void generate_random( int nruns, vect2 imin, vect2 imax, vect2 vmin, vect2 vmax, float ang_offset, cluster *clist)
{
	cluster *uck = clist;
	int run;
	vect2 start;

	for(run = 0; run < nruns; run++ ) {
		start = box_of_random2( vmin, vmax );
		cluster_initialize( uck );

		/* cluster_set_run( 	200, 		// number of elements in run
					start,		// starting point of run 
					0.0625, 	// initial step size
					0.99, 		// proportional change per step
					ang_offset, // motion relative to vector field
					uck );		// pointer to cluster */

		cluster_set_run( 	100, 		// number of elements in run
					start,		// starting point of run 
					0.25, 		// initial step size
					1.0, 		// proportional change per step
					ang_offset, // motion relative to vector field
					uck );		// pointer to cluster 

		cluster_set_bounds( imin, imax, uck );

		/* c_set_color( 	rand1() * 0.33 + 0.33, 		// Initial color
						rand1() * 0.005 - 0.0025, 		// Color increment
						0.5, 		// Initial brightness
						1.0, 		// Brightness proportional change per step
						uck ); */

		// cluster_set_size( true, 2.0, uck );
		cluster_set_size( true, 4.0, uck );

		uck->brightness = 0.5;
		uck->brightness_prop = 1.0;

		uck++;
	}
}

// Set up a simulated aurora
// Future: create a filetype with all the parameters 
void generate_aurora( 	float norm_frame, 
						int nruns, 
						vect2 imin, vect2 imax, 
						vect2 start, 
						float ang_offset, 
						wave_params *ang_wave, // wave_params *brightness_wave,
						palette *p,
						vfield *vf, 
						vect_fn_2d_t_params *vortex_field,
						cluster *clist,
						scene *scn)
{
	if( debug_me ) printf("generate_aurora\n");

	cluster *uck = clist;
	int run;
	float local_offset = 0.0;

	// set up color palette
	int npal = 4;
	float cindex[ npal ];
	frgb cpal[ npal ];
	float brightness;
	int i;
	vect2 perturb_start;

	for(run = 0; run < nruns; run++ ) {
		cluster_initialize( uck );

		if( debug_me ) printf("generate_aurora: perturb\n");

		// perturb starting point using perturbation field
		perturb_start = start;
		if( scn->use_perturb ) {
			for(i=0; i<scn->perturb_steps; i++ ) {
				perturb_start = vect_fn_advect( perturb_start, vortex_field, 0.0625, 0.0, norm_frame );
			}
		}

		if( debug_me ) printf("generate_aurora: cluster_set_run\n");
		cluster_set_run( 	50, 		// number of elements in run
							perturb_start,		// starting point of run 
							0.0625, 	// initial step size
							1.0, 		// proportional size change per step
							ang_offset, // motion relative to vector field
							uck );		// pointer to cluster */

		if( debug_me ) printf("generate_aurora: cluster_set_bounds\n");
		cluster_set_bounds( imin, imax, uck );

		brightness = 0.0625 + rand1() * 0.03125;
		// circle function (fade ends )
		brightness *= sqrt( 1.0 - (( run - nruns / 2.0 ) * ( run - nruns / 2.0 )) / (( nruns / 2.0 ) * ( nruns / 2.0 )));
		//brightness *= 1.0 + 0.3 * sin( (3.0 * frame / 10.0 + run) / 5.0 );
		//brightness *= 1.0 + composed_wave( run, frame, brightness_wave );
		brightness *= 1.0 + pow( rand1(), 4.0 );

		// change to custom palette
		if( debug_me ) printf("generate_aurora: cluster_set_color\n");
		cluster_set_color( 	p,								// Color palette
							0.0 + rand1() * 0.1, 			// Initial color
							0.0133, 						// Color increment
							brightness, 					// Initial brightness
							0.96, 							// Brightness proportional change per step
							uck ); 

		uck->brightness_ramp = true;
		uck->brightness_ramp_length = 5;

		// c_set_size( true, 2.0, uck );
		if( debug_me ) printf("generate_aurora: cluster_set_size\n");
		cluster_set_size( true, 2.0, uck );
		if( scn->use_wave ) local_offset = composed_wave( 1.0 * run / nruns, norm_frame, ang_wave );
		else local_offset = 0.0;

		if( debug_me ) printf("generate_aurora: vfield_advect\n");
		start = vfield_advect( start, vf, 0.0625, local_offset );

		uck++;
	}
}
