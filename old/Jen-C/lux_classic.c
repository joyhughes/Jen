#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

// floating point rgb values can exceed 1 (or be negative "anticolors" for that matter)
// must be normalized and converted back to 24-bit color space to be meaningfully displayed

#define TAU 6.283185307179586

bool critical_run;
bool critical_element;
bool debug_me;

typedef struct Vect2 
{
	float x,y;
} vect2;

typedef struct Vfield
{
	int xdim, ydim;		// Size of image in pixels
	vect2 min, max;		// Bounds of field in cartesian space
	vect2 *f;			// Flow vectors
} vfield;


typedef struct Wave_params
{
	int nwaves;
	float constant;		// Constant value added in
	float *magnitude; 	// magnitude of each wave
	float *offset;
	float *frame_freq;
	float *run_freq;
} wave_params;

typedef struct Vortex
{
	vect2 origin;
	wave_params concentric;
	wave_params rotation;
	bool inverse_square;
	wave_params diameter;
	wave_params soften;				
	wave_params offset_distance;
	wave_params offset_angle;
} vortex;

// eventually will hold all params for a 2d animated vector function
// right now a list of vortices
typedef struct Vect_fn_2d_t_params
{
	int n; 	// number of vortices
	vortex *vortex_list;
} vect_fn_2d_t_params;

typedef struct FRGB
{
	float r,g,b;

} frgb;

typedef struct Palette
{
	int ncolors;
	float *indices;
	frgb *colors;
} palette;

typedef struct FIMAGE
{
	int xdim, ydim;	
	vect2 min, max;		// Bounds of image in cartesian space
	frgb *f;			// Pointer to array of frgb pixels
} fimage;

// Specifies a grouping of subjects with various generative rules
// Avoids functions with a bazillion arguments
typedef struct Cluster
{
	// element *id;		// What am I?
	int n;				// Number of subjects in cluster
	vect2 start;		// Location of first item in cluster

	// vector field associated with cluster 
	// vfield *field;

	// Size properties
	float size;			// Size of object in parametric space
	bool  size_prop;	// Is size proportional to step size?

	// Step properties
	float step;			// Initial step size
	float prop;			// Proportional change in stepsize per step
	bool  bounded;		// Is there a boundary condition?
	vect2 bmin,bmax;	// Bounding rectangle for cluster

	// Move at an angle to vector field
	float ang_offset; // Angle of motion relative to vector field
	// float ang_offset_inc;  
	// other angle offset parameters - proportional to size?

	// Parameters for handling angles (in degrees)
	float ang_init;		// initial angle
	float ang_inc;		// angle increment
	bool  ang_relative;	// Are angles relative to vector direction?

	// Color properties
	palette *color_palette;
	bool  color_filter;		// Filter the splat color?
	bool  brightness_ramp;	// ramp brightness up at beginning?
	int   brightness_ramp_length;
	float start_color;		// starting color in palette function
	float color_inc;   		// color change
	float brightness;		// overall brightness of splat
	float brightness_prop; 	// proportional change in brightness per step
	// other brightness change properties here

	// animation properties
	// pulsation etc.
	//

	// branching properties - spawn new clusters recursively
	struct cluster *subclusters;

} cluster;

// Structure to hold all elements of a scene
typedef struct Scene
{
	char base_file[ 255 ];
	bool use_mask;
	char background_file[ 255 ];
	char mask_file[ 255 ];
	char splat_file[ 255 ];		// Near future: will need multiple splat images
	bool use_stencil;
	float stencil_boost;
	char stencil_file[ 255 ];
	bool reflect_x, reflect_y; 	// Future: add kaleidoscope
	int  reflect_x_line, reflect_y_line;
	char output_basename[255];
	bool use_wave;				// Use composed wave for aurora
	bool use_perturb;
	int perturb_steps;

	// image pointers
	fimage *base_fimg, *background_fimg, *mask_fimg, *splat_fimg, *stencil_fimg;
	
} scene;

// returns a random number between 0 and 1
float rand1()
{
	return (rand() * 1.0) / (RAND_MAX * 1.0);
}

float sin_deg( float theta )
{
	return sin( theta / 360.0 * TAU );
}

void wave_params_initialize( wave_params &wvp )
{
	wvp->nwaves = 0;
	wvp->constant = 0.0;		// Constant value added in
	wvp->magnitude = NULL; 	// magnitude of each wave
	wvp->offset = NULL;
	wvp->frame_freq = NULL;
	wvp->run_freq = NULL;
}

// inputs scaled to [ 0.0 , 1.0 ]
float composed_wave( float run, float frame, wave_params *w )
{
	int i;
	float out = 0.0;

	for( i = 1; i < w->nwaves; i++ ) {
		out += w->magnitude[ i ] * sin_deg( w->offset[ i ] + frame * 360.0 * w->frame_freq[ i ] + run * 360.0 * w->run_freq[ i ] );
	}
	return out;
}

void make_random_wave( int nwaves, float max_magnitude, int max_run_freq, int max_frame_freq, wave_params *w )
{
	int i;

	w->magnitude =  (float *)malloc( nwaves * sizeof(float));
	w->offset =     (float *)malloc( nwaves * sizeof(float));
	w->frame_freq = (float *)malloc( nwaves * sizeof(float));
	w->run_freq =   (float *)malloc( nwaves * sizeof(float));

	w->nwaves = nwaves;

	for( i = 1; i < w->nwaves; i++ ) {
		w->offset[ i ]     = rand1() * 360.0;
		w->magnitude[ i ]  = rand1() * max_magnitude;

		w->frame_freq[ i ] = (float)( ( (rand() % max_frame_freq) + 1 ) * ( ( rand() % 2 ) * 2 - 1 ) );
		w->run_freq[ i ]   = (float)( ( (rand() % max_run_freq) + 1 ) * ( ( rand() % 2 ) * 2 - 1 ) );
	}
}

void free_wave_params( wave_params *w )
{
	free( w->magnitude );
	free( w->offset );
	free( w->frame_freq );
	free( w->run_freq );
}


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


float add_angle( float a1, float a2 )
{
	float a = fmod(a1 + a2, 360.0 );
	if( a < 0.0 ) a += 360.0;
	return a;
}

// rotate a vector by theta degrees
vect2 v_rotate( vect2 in, float theta)
{
	vect2 out;
	float thrad = theta / 360.0 * TAU;

	out.x = in.x * cos( thrad ) - in.y * sin( thrad );
	out.y = in.x * sin( thrad ) + in.y * cos( thrad );

	return out;
}

// Sets the value of vector components
void v_set( vect2 *v, float x, float y)
{
	v->x = x;
	v->y = y;
}

float v_magnitude( vect2 v )
{
	return(sqrt(v.x*v.x + v.y*v.y));
}

// Returns angle corresponding to vector (in degrees)
// Wondering if I should correspond to NESW?
float vtoa( vect2 v )
{
	float ang;

	ang = atan2( v.y, v.x ) / TAU * 360.0;
	if( ang < 0.0 ) ang += 360.0;
	return ang;
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

// returns a vector 90 degrees to the left of input vector
// replace with rotation matrix

vect2 v_complement( vect2 v )
{
	vect2 w;

	w.x = -v.y;
	w.y =  v.x;
	return w;
}

vect2 v_add( vect2 a, vect2 b )
{	
	vect2 w;

	w.x = a.x + b.x;
	w.y = a.y + b.y;
	return w;
}

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

vect2 v_inverse_square( vect2 in, float diameter, float soften )
{
	vect2 out;
	float mag;

	if( ( in.x == 0.0) && ( in.y == 0.0 ) ) {
		v_set( &out, 0.0, 0.0 );
		return out;
	}
	else {
		mag = diameter * diameter * sqrt( 1.0 / ( in.x * in.x + in.y * in.y + soften * soften ) );
		out = v_normalize( in );
		out.x *= mag; out.y *= mag;
		return out;
	}
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


// Set size and allocate memory for vector field
void vfield_initialize( int xsiz, int ysiz, vect2 fmin, vect2 fmax, vfield *vf )
{
	vf->xdim = xsiz;
	vf->ydim = ysiz;
	vf->min = fmin;
	vf->max = fmax;
	vf->f = (vect2 *)malloc( xsiz * ysiz * sizeof(vect2) );
}

// Returns the value of the vector field given integer indices
vect2 vfield_index( int xi, int yi, vfield *vf )
{
	return( vf->f[ (xi % vf->xdim) + vf->xdim * (yi % vf->ydim)]);
}

// Returns coordinate in vector field's linear space given integer indices
vect2 vfield_coord( int xi, int yi, vfield *vf )
{
	vect2 w;

	w.x = xi * ( vf->max.x - vf->min.x ) / ( 1.0 * vf->xdim ) + vf->min.x;
	w.y = yi * ( vf->max.y - vf->min.y ) / ( 1.0 * vf->ydim ) + vf->min.y;

	return w;
}

// Returns interpolated value of vector field given floating point indices
// Still has issues with out of bounds values - should wrap, but sometimes crashes
vect2 vfield_smooth_index( vect2 v, vfield *vf )
{
	vect2 w,u00,u01,u10,u11,u;
	int xi, yi;		// Integer part of index
	float xf, yf;	// Fractional part of index

	// calculate index of vector in rational space
	// out of bounds vectors just wrap around - index will take the modulus
	w.x = (v.x - vf->min.x) / (vf->max.x - vf->min.x) * vf->xdim;
	w.y = (v.y - vf->min.y) / (vf->max.y - vf->min.y) * vf->ydim;

	// locate bounding cell of vector 
	xi = floor(w.x);
	xf = w.x - xi * 1.0;
	yi = floor(w.y);
	yf = w.y - yi * 1.0;

	// linearly interpolate beween surrounding corners
	u00 = vfield_index( xi,   yi,   vf );
	u01 = vfield_index( xi,   yi+1, vf );
	u10 = vfield_index( xi+1, yi,   vf );
	u11 = vfield_index( xi+1, yi+1, vf );

	u.x = (u00.x * ( 1.0 - xf ) + u10.x * xf) * (1.0 - yf) + (u01.x * ( 1.0 - xf ) + u11.x * xf) * yf;
	u.y = (u00.y * ( 1.0 - xf ) + u10.y * xf) * (1.0 - yf) + (u01.y * ( 1.0 - xf ) + u11.y * xf) * yf;

	return u;
}

// Use Newton's method to move along flow line proportional to step value
vect2 vfield_advect( vect2 v, vfield *f, float step, float angle)
{
	vect2 w,u;

	w = vfield_smooth_index( v, f );
	w = v_rotate( w, angle );
	u.x = v.x + step * w.x;
	u.y = v.y + step * w.y;

	return u;
}

// free memory associated with vector field
void vfield_free( vfield *f )
{
	free(f->f);
}

// Return a vector field with each vector 90 degrees to the left
void vfield_complement( vfield *f)
{
	int xi, yi;
	vect2 *w = f->f;

	for(yi = 0; yi < f->ydim; yi++ )
	{
		for(xi = 0; xi < f->xdim; xi++ )
		{
			*w = v_complement(*w);
			w++;
		}
	}
}

// scales a vector field by factor s
void vfield_scale( float s, vfield *f)
{
	int xi, yi;
	vect2 *w = f->f;

	for(yi = 0; yi < f->ydim; yi++ )
	{
		for(xi = 0; xi < f->xdim; xi++ )
		{
			w->x *= s; w->y *= s;
			w++;
		}
	}
}

// scales a vector field by factor s
void vfield_rotate_vectors( float ang, vfield *f)
{
	int xi, yi;
	vect2 *w = f->f;

	for(yi = 0; yi < f->ydim; yi++ )
	{
		for(xi = 0; xi < f->xdim; xi++ )
		{
			*w = v_rotate( *w, ang );
			w++;
		}
	}
}

// Return sum of two vector fields
// assume all of same dimension
void vfield_sum( vfield *a, vfield *b, vfield *f)
{
	int xi, yi;
	vect2 *w =  f->f;
	vect2 *wa = a->f;
	vect2 *wb = b->f;

	// Sanity check - are they the same dimension?
	if( (a->xdim != b->xdim) | (a->xdim != f->xdim) | (a->ydim != b->ydim) | (a->ydim != f->ydim) )
	{
		printf("Error f_sum expects fields of same dimension\n");
	}
	else 
	{
		for(yi = 0; yi < f->ydim; yi++ )
		{
			for(xi = 0; xi < f->xdim; xi++ )
			{
				w->x = wa->x + wb->x;
				w->y = wa->y + wb->y;
				w++; wa++; wb++;
			}
		}
	}
}

// Return sum of first vector field and complement of second vector field
// assume all of same dimension
void vfield_add_complement( vfield *a, vfield *b, vfield *f)
{
	int xi, yi;
	vect2 *w =  f->f;
	vect2 *wa = a->f;
	vect2 *wb = b->f;

	// Sanity check - are they the same dimension?
	if( (a->xdim != b->xdim) | (a->xdim != f->xdim) | (a->ydim != b->ydim) | (a->ydim != f->ydim) )
	{
		printf("Error f_sum expects fields of same dimension\n");
	}
	else 
	{
		for(yi = 0; yi < f->ydim; yi++ )
		{
			for(xi = 0; xi < f->xdim; xi++ )
			{
				w->x = wa->x - wb->y;
				w->y = wa->y + wb->x;
				w++; wa++; wb++;
			}
		}
	}
}

// Creates an inverse square of vector field with optional softening factor to avoid infinity 
void vfield_inverse_square( float diameter, float soften, vfield *f )
{
	int xi, yi;
	vect2 *w = f->f;
	vect2 u;
	float s;

	for(yi = 0; yi < f->ydim; yi++ )
	{
		for(xi = 0; xi < f->xdim; xi++ )
		{
			if((diameter==0.0) | ( (soften==0.0) & (w->x==0.0) & (w->y==0.0)))
			{
				w->x = 0.0; w->y=0.0;
			}
			else
			{
				s = v_magnitude(*w)/diameter + soften;
				s = 1.0 / (s*s);
				u = v_normalize(*w);
				w->x = u.x * s; w->y = u.y * s; 
			}
			w++;
		}
	}
}

// Normalizes every vector in the field
void vfield_normalize( vfield *f )
{
	int xi, yi;
	vect2 *w = f->f;

	for(yi = 0; yi < f->ydim; yi++ )
	{
		for(xi = 0; xi < f->xdim; xi++ )
		{
			*w = v_normalize(*w);
			w++;
		}
	}
}

// Copy same vector into every member of vector field
void vfield_linear( vect2 v, vfield *f)
{
	int xi, yi;
	vect2 *w = f->f;

	for(yi = 0; yi < f->ydim; yi++ )
	{
		for(xi = 0; xi < f->xdim; xi++ )
		{
			*w = v;
			w++;
		}
	}
}

// Add same vector to every member of vector field
void vfield_add_linear( vect2 v, vfield *f)
{
	int xi, yi;
	vect2 *w = f->f;

	for(yi = 0; yi < f->ydim; yi++ )
	{
		for(xi = 0; xi < f->xdim; xi++ )
		{
			w->x += v.x;			
			w->y += v.y;
			w++;
		}
	}
}

// creates an array of vectors pointing from center
void vfield_concentric( vect2 center, vfield *f )
{
	int xi, yi;
	vect2 *w = f->f;
	vect2 u;

	for(yi = 0; yi < f->ydim; yi++ )
	{
		for(xi = 0; xi < f->xdim; xi++ )
		{
			u = vfield_coord( xi, yi, f );
			w->x = u.x - center.x;
			w->y = u.y - center.y;
			w++;
		}
	}
}

// creates an array of vectors rotating around center
void vfield_rotation( vect2 center, vfield *f )
{
	// rotation field is complement of concentric field
	vfield_concentric( center, f );
	vfield_complement( f );
}

// creates a spiraling vector field by combining concentric and rotational fields in proportion
void vfield_spiral( vect2 center, float cscale, float rscale, vfield *f )
{
	vfield g;
	vfield_initialize( f->xdim, f->ydim, f->min, f->max, &g );	// initialize buffer

	vfield_concentric( center, f );
	vfield_scale( cscale, f );
	vfield_rotation( center, &g );
	vfield_scale( rscale, &g );
	vfield_sum( f, &g, f );

	vfield_free( &g );		// free memory used in buffer
}

// Adds a constant vector over box shaped area into a vector field
void vfield_box( vect2 min, vect2 max, vect2 v, vfield *f )
{
	int xi, yi;
	vect2 *w = f->f;

	for(yi = 0; yi < f->ydim; yi++ )
	{
		for(xi = 0; xi < f->xdim; xi++ )
		{
			if( in_bounds( vfield_coord( xi, yi, f ), min, max, 0.0 ) )
			{
				w->x += v.x;
				w->y += v.y;
			}
			w++;
		}
	}
}

void vfield_add_stripes( vfield *f )
{
	int n=4;	// four jags /* plus one to rule them all */
	int i;
	float w;	// width of stripe
	float stripex;	// x position of stripe
	vect2 v, min, max;
	float c, vv;

	for( i=0; i<n; i++ )
	{
		w = 1.0;
		stripex = 2.0 * i + 2.0;
		v.x = 0.0; 
		v.y = rand1() * 8.0 - 4.0;
		min.x = stripex - w;
		min.y = f->min.y;
		max.x = stripex;
		max.y = f->max.y;
		vfield_box( min, max, v, f );
		min.x = stripex;
		max.x = stripex + w;
		v.y = -1.0 * v.y;
		vfield_box( min, max, v, f );
	}
}

// Composite vector field including multiple centers of rotation
void vfield_turbulent( float diameter, int n, bool random_rotation, vfield *f)
{
	int i;
	vect2 w,c;
	vfield g;	// buffer to hold each center of rotation before adding in

	// Set value of output array to zero
	w.x= 0.0; w.y=0.0;
	vfield_linear(w,f);

	vfield_initialize( f->xdim, f->ydim, f->min, f->max, &g );	// initialize buffer
	for(i=0; i<n; i++)
	{
		c = box_of_random2( g.min, g.max );
		vfield_rotation( c, &g );
		if( random_rotation ) vfield_rotate_vectors( rand1() * 360.0, &g);	// rotate vectors in random direction
		else if( rand()%2 ) vfield_scale( -1.0, &g );		// coin flip for rotational direction
		
		vfield_inverse_square( diameter, 0.5, &g );
		vfield_sum( f, &g, f );
	}
	vfield_free( &g );		// free memory used in buffer
}

void vortex_initialize( vortex *vort )
{

}

void vortex_free( vortex *vort )
{
	if( )
}

// procedural animated 2d vector function
// future: generalize this by passing a function tree
// at the moment uses a sum of vortices
vect2 vect_fn_2d_t( vect2 v, vect_fn_2d_t_params *params, float t )
{
	vortex *vort = params->vortex_list;
	vect2 position, w;
	vect2 out; v_set( &out, 0.0, 0.0 );
	int i;
	int n=params->n;

	for( i=0; i<n; i++ ) {
		// calculate position of vortex
		position = v_add( vort->center_of_revolution, v_rotate( vort->start_offset, 360.0 * t * vort->revolution_velocity ) );
		w = v_subtract( v, position );
		w = v_inverse_square( w, vort->diameter, vort->soften );
		w = v_complement( w );
		w = v_scale( w, vort->speed_of_rotation ); 
		out = v_add( w, out );
		vort++;
	}

	out = v_normalize( out );
	return out;
}

void free_vect_fn_2d_t_params( vect_fn_2d_t_params *params )
{
	free( params->vortex_list );
}

// Use Newton's method to move along flow line proportional to step value
vect2 vect_fn_advect( vect2 v, vect_fn_2d_t_params *params, float step, float angle, float t)
{
	vect2 w,u;

	w = vect_fn_2d_t( v, params, t );
	w = v_rotate( w, angle );
	u.x = v.x + step * w.x;
	u.y = v.y + step * w.y;
	if( debug_me ) printf(" vect_fn_advect: w = ( %f, %f )\n", w.x, w.y );

	return u;
}

// remove_ext: removes the "extension" from a file spec.
//   myStr is the string to process.
//   extSep is the extension separator.
//   pathSep is the path separator (0 means to ignore).
// Returns an allocated string identical to the original but
//   with the extension removed. It must be freed when you're
//   finished with it.
// If you pass in NULL or the new string can't be allocated,
//   it returns NULL.

char *remove_ext (char* myStr, char extSep, char pathSep) {
    char *retStr, *lastExt, *lastPath;

    // Error checks and allocate string.

    if (myStr == NULL) return NULL;
    if ((retStr = malloc (strlen (myStr) + 1)) == NULL) return NULL;

    // Make a copy and find the relevant characters.

    strcpy (retStr, myStr);
    lastExt = strrchr (retStr, extSep);
    lastPath = (pathSep == 0) ? NULL : strrchr (retStr, pathSep);

    // If it has an extension separator.

    if (lastExt != NULL) {
        // and it's to the right of the path separator.

        if (lastPath != NULL) {
            if (lastPath < lastExt) {
                // then remove it.

                *lastExt = '\0';
            }
        } else {
            // Has extension separator with no path separator.

            *lastExt = '\0';
        }
    }

    // Return the modified string.

    return retStr;
}

// linear interpolation of colors through a palette
// array should have colors at 0.0 and 1.0
// color indicies in increasing order, no two the same
frgb palette_index( float a, palette *p )
{
	frgb c;
	int i;
	float blend;

	a = a - floor(a);

	for( i=0; i < p->ncolors-1; i++ ) {
		if( ( a >= p->indices[i] ) && ( a < p->indices[i+1] ) ) {
			blend = ( a - p->indices[i] ) / ( p->indices[i+1] - p->indices[i] );
			c.r = ( 1.0 - blend ) * p->colors[i].r + blend * p->colors[i+1].r;
			c.g = ( 1.0 - blend ) * p->colors[i].g + blend * p->colors[i+1].g;
			c.b = ( 1.0 - blend ) * p->colors[i].b + blend * p->colors[i+1].b;
		}
	}
	return c;
}

// deprecated - use rainbow_palette instead
frgb rainbow( float a )
{
	frgb c;

	// wrap around
	a = a - floor(a);

	a *= 6.0;

	if(a < 1.0) 
	{
		// red to yellow
		c.r = 1.0;
		c.g = a;
		c.b = 0.0;
	}
	else if( a < 2.0 )
	{
		// yellow to green
		c.r = 2.0 - a;
		c.g = 1.0;
		c.b = 0.0;
	}
	else if( a < 3.0 )
	{
		// green to cyan
		c.r = 0.0;
		c.g = 1.0;
		c.b = a - 2.0;
	}
	else if( a < 4.0 )
	{
		// cyan to blue
		c.r = 0.0;
		c.g = 4.0 - a;
		c.b = 1.0;
	}
	else if( a < 5.0 )
	{
		// blue to magenta
		c.r = a - 4.0;
		c.g = 0.0;
		c.b = 1.0;
	}
	else
	{
		// magenta to red
		c.r = 1.0;
		c.g = 0.0;
		c.b = 6.0 - a;
	}

//	printf("rainbow a=%02f\n r=%f\n g=%f\n b=%f\n",a,c.r,c.g,c.b);
	return c;
}

void fimage_free( fimage *f )
{
	free( f->f );
}

frgb fcolor( unsigned char r, unsigned char g, unsigned char b )
{
	frgb f;

	// here can also raise to the power of gamma
	f.r = r / 255.0;
	f.g = g / 255.0;
	f.b = b / 255.0;

	return f;
}

// presume here f is already initialized - that way we can reuse buffer
void fimage_continuous( int channels, unsigned char *img, fimage *f )
{
	int x, y;
	unsigned char *c = img;
	frgb *p = f->f;
	int xdim = f->xdim;
	int ydim = f->ydim;

	for( y = 0; y < ydim; y++ )
	{
		for( x = 0; x < xdim; x++ )
		{
			if( channels == 1 )	// monochrome image
			{
				p->r = *c / 255.0;
				p->g = *c / 255.0;
				p->b = *c / 255.0;
				c++;
			}

			if( channels == 2 ) // don't know what to do in this case
			{
				p->r = *c / 255.0;
				p->g = *c / 255.0;
				p->b = *c / 255.0;
				c++; c++;
			}

			if( ( channels == 3 ) | ( channels == 4 ) )
			{
				p->r = *c / 255.0;
				c++;
				p->g = *c / 255.0;
				c++;
				p->b = *c / 255.0;
				c++;
			}

			// skip alpha channel - rgba ... if argb need to move line up
			if( channels == 4 ) c++;	
			p++;
		}
	}
}

// fimage must already be initialized
// img must already be allocated
void fimage_quantize( unsigned char *img, fimage *f )
{
	int x, y;
	unsigned char *c;
	frgb *p = f->f;
	int xdim = f->xdim;
	int ydim = f->ydim;

	c = img;

	for( y = 0; y < ydim; y++ )
	{
		for( x = 0; x < xdim; x++ )
		{
			*c = p->r * 0xff;
			c++;
			*c = p->g * 0xff;
			c++;
			*c = p->b * 0xff;
			c++;
			//*c = 0xff;	// Alpha channel - may need to put this above
			//c++;
			p++;
		}
	}
}

void fimage_set_bounds(  vect2 min, vect2 max, fimage *f )
{
	f->min = min;
	f->max = max;
}

void fimage_init( int xdim, int ydim, fimage *f )
{
	vect2 min; min.x = -1.0; min.y = -1.0;
	vect2 max; max.x =  1.0; max.y =  1.0;

	f->xdim = xdim;
	f->ydim = ydim;

	f->min = min;
	f->max = max;

	f->f = (frgb *)malloc( xdim * ydim * sizeof(frgb) );
}

int fimage_load( char *filename, fimage *fimg )
{
	unsigned char *img;
	int channels, xdim, ydim;

    img = stbi_load( filename, &xdim, &ydim, &channels, 0 );
    if(img == NULL) 
	{
    	printf( "Error in loading image %s\n", filename );
    	return 0;
	}

	printf( " image %s loaded\n", filename );
	fimage_init( xdim, ydim, fimg );
	fimage_continuous( channels, img, fimg );	// convert image to frgb
	free( img );
	return 1;
}

void fimage_write_jpg(const char *filename, fimage *fimg )
{
    unsigned char *img = (unsigned char *)malloc( 3 * fimg->xdim * fimg->ydim * sizeof( unsigned char ) );

	fimage_quantize( img, fimg );
	stbi_write_jpg( filename, fimg->xdim, fimg->ydim, 3, img, 100);

	free( img );
}

void fimage_write_ppm( char *filename, fimage *fimg )
{
    FILE *fp;
    unsigned char *img = (unsigned char *)malloc( 3 * fimg->xdim * fimg->ydim * sizeof( unsigned char ) );

    fimage_quantize( img, fimg );

    //open file for output
    fp = fopen( filename, "wb" );
    if ( !fp ) {
         printf( "Unable to open file '%s'\n", filename );
         exit(1);
    }

    //write the header file
    //image format
    fprintf(fp, "P6\n");

    //comments
    fprintf(fp, "# Created by Joyographic\n");

    //image size
    fprintf( fp, "%d %d\n", fimg->xdim, fimg->ydim );

    // rgb component depth
    fprintf(fp, "%d\n",255);

    // pixel data
    fwrite( img, 3 * fimg->xdim, fimg->ydim, fp);
    fclose(fp);
}

// copies only the contents of images
// error if images different sizes
void fimage_copy_contents( fimage *in, fimage *out)
{
	int x,y;
	frgb *pin = in->f;
	frgb *pout = out->f;
	int xdim = in->xdim;
	int ydim = in->ydim;

	if( ( out->xdim != in->xdim ) || ( out->ydim != in->ydim ) ) 
		printf("fimage_copy_contents error - dimensions\n in %d %d out %d %d\n", in->xdim, in->ydim, out->xdim, out->ydim);

	for( y = 0; y < ydim; y++ ) {
		for( x = 0; x < xdim; x++ ) {
			*pout = *pin;
			pin++;
			pout++;
		}
	}
}

void fimage_init_duplicate( fimage *in, fimage *out )
{
	fimage_init( in->xdim, in->ydim, out);
	fimage_set_bounds( in->min, in->max, out );
	fimage_copy_contents( in, out );
}

void fimage_reflect_y( int mirror, bool top_to_bottom, fimage *in, fimage *out)
{
	int x,y;
	frgb *pin = in->f;
	frgb *pout = out->f;
	frgb *preflect;
	int xdim = in->xdim;
	int ydim = in->ydim;
	int rrow;
	bool reflect;

	if( top_to_bottom ) {
		for( y = 0; y <= mirror; y++ ) {
			rrow = (mirror - y) + mirror;
			reflect = rrow < ydim;
			if( reflect ) preflect = out->f + rrow * xdim;
			for( x = 0; x < xdim; x++ ) {
				*pout = *pin;
				if( reflect ) *preflect = *pin;
				pin++;
				pout++;
				if( reflect ) preflect++;
			}
		}
	}
	// fill in bottom to top caseß
}

// assume all images are the same size
void fimage_sum( fimage *a, fimage *b, fimage *result )
{
	int x,y;
	frgb *pa = a->f;
	frgb *pb = b->f;
	frgb *pr = result->f;
	int xdim = a->xdim;
	int ydim = a->ydim;

	for( y = 0; y < ydim; y++)	
	{
		for( x = 0; x < xdim; x++)
		{
			pr->r = pa->r + pb->r;
			pr->g = pa->g + pb->g;
			pr->b = pa->b + pb->b;

			pa++;	pb++;	pr++;
		}
	}
}

// assume all images are the same size
// clip components at zero - no negative results
void fimage_subtract( fimage *a, fimage *b, fimage *result )
{
	int x,y;
	frgb *pa = a->f;
	frgb *pb = b->f;
	frgb *pr = result->f;
	int xdim = a->xdim;
	int ydim = a->ydim;

	for( y = 0; y<ydim; y++)	
	{
		for( x = 0; x<xdim; x++)
		{
			pr->r = pa->r - pb->r;
			if( pr->r < 0.0 ) pr->r = 0.0;
			pr->g = pa->g - pb->g;
			if( pr->g < 0.0 ) pr->g = 0.0;
			pr->b = pa->b - pb->b;
			if( pr->b < 0.0 ) pr->b = 0.0;

			pa++;	pb++;	pr++;
		}
	}
}

void fimage_fill( frgb color, fimage *f )
{
	int x,y;
	frgb *p = f->f;
	int xdim = f->xdim;
	int ydim = f->ydim;

	for( y = 0; y<ydim; y++)	
	{
		for( x = 0; x<xdim; x++)
		{
			p->r = color.r;
			p->g = color.g;
			p->b = color.b;

			p++;
		}
	}
}

void fimage_color_filter( frgb color, fimage *f )
{
	int x,y;
	frgb *p = f->f;
	int xdim = f->xdim;
	int ydim = f->ydim;

	for( y = 0; y < ydim; y++)	
	{
		for( x = 0; x < xdim; x++)
		{
			p->r *= color.r;
			p->g *= color.g;
			p->b *= color.b;

			p++;
		}
	}
}

void fimage_square( fimage *f )
{
	int x,y;
	frgb *p = f->f;
	int xdim = f->xdim;
	int ydim = f->ydim;

	for( y = 0; y<ydim; y++)	
	{
		for( x = 0; x<xdim; x++)
		{
			p->r *= p->r * p->r;
			p->g *= p->g * p->g;
			p->b *= p->b * p->b;

			p++;
		}
	}
}

void fimage_normalize( fimage *f )
{
	int x,y;
	frgb *p = f->f;
	float max = 0.0;
	int xdim = f->xdim;
	int ydim = f->ydim;

	// scan for maximum value
	for( y = 0; y < ydim; y++ )	
	{
		for( x = 0; x < xdim; x++ )
		{
			if ( p->r > max ) max = p->r;
			if ( p->g > max ) max = p->g;
			if ( p->b > max ) max = p->b;

			p++;
		}
	}

	if( max > 0.0 )
	{
		p = f->f;
		for( y = 0; y < ydim; y++ )	
		{
			for( x = 0; x < xdim; x++) 
			{
				p->r /= max;
				p->g /= max;
				p->b /= max;

				p++;
			}
		}
	}
}

// look up pixel by integer indices
frgb fimage_index( int x, int y, fimage *f )
{
	frgb black;

	if( (x>=0) & (x<f->xdim) & (y>=0) & (y<f->ydim) ) 
	{
		return( *( f->f + y * f->xdim + x ) );
	}
	else
	{
		black.r = 0.0; black.g = 0.0; black.b = 0.0;
		return black;
	}
}

// sample image given coordinate in linear space 
// quick and dirty gives nearest pixel index
// near future: more elegant - smoothed blend (future: multi resolution)
frgb fimage_sample( vect2 v, bool smooth, fimage *fimg )
{
	frgb black;

	int x =  fimg->xdim * ( v.x - fimg->min.x ) / ( fimg->max.x - fimg->min.x );
	int y =  fimg->ydim * ( v.y - fimg->min.y ) / ( fimg->max.y - fimg->min.y );

	if( ( x>=0 ) & ( x<fimg->xdim ) & ( y>=0 ) & ( y<fimg->ydim ) ) 
	{
		return( *( fimg->f + y * fimg->xdim + x ) );
	}
	else
	{
		black.r = 0.0; black.g = 0.0; black.b = 0.0;
		return black;
	}
}

// in ang out images must be separate
void fimage_rotate( float theta, 
					vect2 cor, 					// Center of rotation
					fimage *in, fimage *out )
{
	int x, y, ox, oy;
	float fx, fy, fox, foy;
	frgb *o = out->f;
	int xdim = in->xdim;
	int ydim = in->ydim;

	float cth = cos( -theta );	// ulhu
	float sth = sin( -theta );

	for( y = 0; y < ydim; y++ )
	{
		for( x = 0; x < xdim; x++ )
		{
			fx = x * 1.0 - cor.x;
			fy = y * 1.0 - cor.y;
			fox = fx * cth - fy * sth;
			foy = fx * sth + fy * cth;
			ox = (int)(fox + cor.x);
			oy = (int)(foy + cor.y);

			*o = fimage_index( ox, oy, in );
			o++;
		}
	}
}

// in ang out images must be separate
void fimage_translate( int xoff, int yoff, fimage *in, fimage *out )
{
	int x, y, ox, oy;
	frgb *o = out->f;
	int xdim = in->xdim;
	int ydim = in->ydim;

	for( y=0; y<ydim; y++ )
	{
		for( x=0; x<xdim; x++ )
		{
			ox = x - xoff;
			oy = y - yoff;
			*o = fimage_index( ox, oy, in );
			o++;
		}
	}
}

// Colors black everything outside of a centered circle
void fimage_circle_crop( fimage *f )
{	
	float r2;
	frgb *p = f->f;
	int x, y;
	float fx, fy;
	int xdim = f->xdim;
	int ydim = f->ydim;

	if( xdim < ydim )
	{
		r2 = xdim / 2.0;
		r2 = r2 * r2;
	}
	else
	{
		r2 = ydim / 2.0;
		r2 = r2 * r2;		
	}

	for( y = 0; y < ydim; y++ )	
	{
		fy = y - ydim / 2.0;
		for( x = 0; x < xdim; x++ )
		{
			fx = x - xdim / 2.0;
			if( r2 < fx*fx + fy*fy )
			{
				p->r = 0.0;
				p->g = 0.0;
				p->b = 0.0;
			}
			p++;
		}
	}
}

void fimage_circle_ramp( fimage *f )
{	
	float r,r1,r2;
	frgb *p = f->f;
	int x, y;
	float fx, fy;
	int xdim = f->xdim;
	int ydim = f->ydim;

	if( xdim < ydim )
	{
		r2 = xdim / 2.0;
		r2 = r2 * r2;
	}
	else
	{
		r2 = ydim / 2.0;
		r2 = r2 * r2;		
	}

	for( y = 0; y < ydim; y++ )	
	{
		fy = y - ydim / 2.0;
		for( x = 0; x < xdim; x++ )
		{
			fx = x - xdim / 2.0;
			r1 = fx*fx + fy*fy;
			if( r1 > r2 )
			{
				p->r = 0.0;
				p->g = 0.0;
				p->b = 0.0;
			}
			else
			{
				r = sqrt( r1 / r2 );
				p->r *= ( 1.0 - r );
				p->g *= ( 1.0 - r );
				p->b *= ( 1.0 - r );
			}
			p++;
		}
	}
}

// future - add antialiasing
void fimage_make_mask( float thresh, fimage *in, fimage *out )
{
	int x,y;
	frgb *pin = in->f;
	frgb *pout = out->f;
	int xdim = in->xdim;
	int ydim = in->ydim;

	for( y = 0; y<ydim; y++ ) {
		for( x = 0; x<xdim; x++ ) {
			if( /* pin->r + pin->g +*/ pin->b > thresh ) {
				pout->r = 1.0; pout->g = 1.0; pout->b = 1.0;
			}
			else {
				pout->r = 0.0; pout->g = 0.0; pout->b = 0.0;
			}
			pin++;
			pout++;
		}
	}
}

void fimage_apply_mask( fimage *base, fimage *mask, fimage *result )
{
	int x,y;
	frgb *pbase = base->f;
	frgb *pmask = mask->f;
	frgb *presult = result->f;
	int xdim = base->xdim;
	int ydim = base->ydim;

	for( y = 0; y < ydim; y++ ) {
		for( x = 0; x < xdim; x++ ) {
			presult->r = presult->r * pmask->r + pbase->r * ( 1.0 - pmask->r);
			presult->g = presult->g * pmask->g + pbase->g * ( 1.0 - pmask->g);
			presult->b = presult->b * pmask->b + pbase->b * ( 1.0 - pmask->b);
			pbase++;
			pmask++;
			presult++;
		}
	}
}

void fimage_clip( float min, float max, fimage *in )
{
	int x,y;
	frgb *p = in->f;
	int xdim = in->xdim;
	int ydim = in->ydim;

	for( y = 0; y < ydim; y++ ) {
		for( x = 0; x < xdim; x++ ) {
			if( p->r < min ) p->r = min;
			if( p->r > max ) p->r = max;
			if( p->g < min ) p->g = min;
			if( p->g > max ) p->g = max;
			if( p->b < min ) p->b = min;
			if( p->b > max ) p->b = max;
			p++;
		}
	}
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
void scene_read( const char *filename, scene *scn )
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

// Future - include an alpha channel mask in the splat image
// Future - use multiple resolutions for sampling to limit aliasing
void fimage_splat( 
	vect2 center, 			// coordinates of splat center
	float scale, 			// radius of splat
	float theta, 			// rotation in degrees
	frgb tint,				// change the color of splat
	fimage *f, 				// image to be splatted upon
	fimage *g 				// image of the splat
	)
{

	// printf("Splat: center.x = %f, center.y = %f\n", center.x, center.y );

	int x,y;
	int findex, gindex;
	int fxdim = f->xdim;
	int fydim = f->ydim;
	int gxdim = g->xdim;
	int gydim = g->ydim;
	frgb *fpix = f->f;
	frgb *gpix = g->f;

	// calculate center of splat in pixel coordinates
	int px,py;
	px = (center.x - f->min.x) / (f->max.x - f->min.x) * fxdim;
	py = (center.y - f->min.y) / (f->max.y - f->min.y) * fydim;

	// calculate scale in pixel coordinates
	int size = scale / (f->max.x - f->min.x) * fxdim;

	// calculate bounding box of splat
	int xmin = px - size;
	int xmax = px + size;
	int ymin = py - size;
	int ymax = py + size;

	// get exact coord of xmin, ymin in splat's linear space
	vect2 smin;
	smin.x = ( 1.0 * xmin ) / fxdim * ( f->max.x - f->min.x ) + f->min.x;
	smin.y = ( 1.0 * ymin ) / fydim * ( f->max.y - f->min.y ) + f->min.y;

	vect2 sc,sf;
	sc.x = (smin.x - center.x) / scale;
	sc.y = -(smin.y - center.y) / scale;
	sc = v_rotate( sc, -theta );

	// calculate unit vectors - one pixel long
	vect2 unx, uny;

	unx.x = 1.0 / fxdim * (f->max.x - f->min.x) / scale;
	unx.y = 0.0;
	unx = v_rotate( unx, -theta );

	uny.x = 0.0;
	uny.y = -1.0 / fydim * (f->max.y - f->min.y) / scale;
	uny = v_rotate( uny, -theta );

	// convert vectors to splat pixel space - fixed point
	int scx =  (int)(( sc.x * g->xdim / 2.0 + g->xdim / 2.0) * 65536.0);
	int scy =  (int)(( sc.y * g->ydim / 2.0 + g->ydim / 2.0) * 65536.0);
	int unxx = (int)( unx.x * g->xdim / 2.0 * 65536.0);
	int unxy = (int)( unx.y * g->ydim / 2.0 * 65536.0);
	int unyx = (int)( uny.x * g->xdim / 2.0 * 65536.0);
	int unyy = (int)( uny.y * g->ydim / 2.0 * 65536.0);
	int sfx, sfy;
	int xdimfix = ( g->xdim - 1 )<<16;
	int ydimfix = ( g->ydim - 1 )<<16;

	// *** Critical loop below ***
	// Quick and dirty sampling, high speed but risk of aliasing. 
	// Should work best if splat is fairly large and smooth.

	for( x = xmin; x < xmax; x++ ) {
		if( (x >= 0) && (x < fxdim) ) {
			sfx = scx;
			sfy = scy;
			for( y = ymin; y < ymax; y++ ) {
				if( (y >= 0) && (y < fydim) && (sfx >= 0) && (sfy >= 0) && (sfx < xdimfix) && (sfy < ydimfix) ) {
					// if( critical_run && critical_element ) printf(" x = %d  y = %d  sfx>>16 = %d  sfy>>16 = %d\n",x,y,sfx >> 16,sfy >> 16);
					findex = y * fxdim + x;
					gindex = (sfy >> 16) * gxdim + (sfx >> 16);
					fpix[ findex ].r += gpix[ gindex ].r * tint.r;
					fpix[ findex ].g += gpix[ gindex ].g * tint.g;
					fpix[ findex ].b += gpix[ gindex ].b * tint.b;
				}
				sfx += unyx;
				sfy += unyy;
			}
		}
		scx += unxx;
		scy += unxy;
	}
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

// Sanity check to see if splat is working
// needs to include tint
void test_splat( fimage *r, fimage *f )
{
	// splat it
	vect2 center;
	center.x = 5.0;
	center.y = 5.0;
//	fimage_splat( center, 1.0, 30.0, &r, &f );
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

void aurora_palette( palette *p )
{
	p->ncolors = 4;
	p->indices = (float *)malloc( p->ncolors * sizeof(float) );
	p->colors  =  (frgb *)malloc( p->ncolors * sizeof(frgb) );

	p->indices[0]  = 0.0;
	p->colors[0].r = 0.0;
	p->colors[0].g = 1.0;
	p->colors[0].b = 0.0;

	p->indices[1]  = 0.5;
	p->colors[1].r = 0.0;
	p->colors[1].g = 0.0;
	p->colors[1].b = 1.0;

	p->indices[2]  = 0.75;
	p->colors[2].r = 1.0;
	p->colors[2].g = 0.0;
	p->colors[2].b = 2.0;

	p->indices[3]  = 1.0;
	p->colors[3].r = 0.0;
	p->colors[3].g = 1.0;
	p->colors[3].b = 0.0;
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

int main( int argc, char const *argv[] )
{
    int channels;
    fimage base, background_buffer, background, splat, result, mask, stencil;
    fimage *render_to;	// pointer to image receiving render
    char *output_filename;
    int i,a;
    vect2 bound1, bound2, vbound1, vbound2;
    time_t t;
    palette color_palette;
    scene scn;
    vect_fn_2d_t_params vortex_field;

    int load_err;			// Image load error code

    vect2 unitx; unitx.x = 1.0; unitx.y = 0.0;
    vect2 unity; unity.x = 0.0; unity.y = 1.0;

    critical_run = false;
    critical_element = false;
    debug_me = false;

	// start random engines
	// useful to keep things repeatable during testing
	//srand((unsigned) time(&t));
	srand( 13 );

    frgb black;	black.r = 0.0;	black.g = 0.0;	black.b = 0.0;
    frgb white;	white.r = 1.0;	white.g = 1.0;	white.b = 1.0;

    scene_initialize( &scn );

    // load scene file
	if( argc >= 2 ) {
		scene_read( argv[ 1 ], &scn );
	}
	else { 
      printf("Usage: ./lux [scene].json [nframes]\n");
      return 0; 
   	}

   	int nframes = 1;
   	if( argc >= 3) nframes = atoi( argv[ 2 ] );

	// allocate output filename with room for code and extension
	output_filename = (char*)malloc( strlen( scn.output_basename ) + 12 );    

   	// load base image
   	// initialize base image
	// origin in lower left

	load_err = fimage_load( scn.base_file, &base );								if( !load_err ) return 0;
	scn.base_fimg = &base;

	v_set( &bound1, 0.0, 10.0 * (1.0 * base.ydim) / (1.0 * base.xdim) );
	v_set( &bound2, 10.0, 0.0 );

	fimage_set_bounds( bound1, bound2, &base );
	fimage_init_duplicate( &base, &result );		// Duplicate base to result

	if( scn.use_mask ) {
		// load background image, duplicate to background buffer
		load_err = fimage_load( scn.background_file, &background );		if( !load_err ) return 0;
		scn.background_fimg = &background;
		fimage_set_bounds( bound1, bound2, &background );
	 	fimage_init_duplicate( &background, &background_buffer );		
	 	render_to = &background_buffer;

		load_err = fimage_load( scn.mask_file, &mask );					if( !load_err ) return 0;
		scn.mask_fimg = &mask;
		fimage_set_bounds( bound1, bound2, &mask );
	}
	else {
		render_to = &result;
	}

	if( scn.use_stencil ) {
		load_err = fimage_load( scn.stencil_file, &stencil );			if( !load_err ) return 0;
		scn.stencil_fimg = &stencil;
		fimage_set_bounds( bound1, bound2, &stencil );
	}

	// load splat image (use default bounds)
   	load_err = fimage_load( scn.splat_file, &splat );					if( !load_err ) return 0;

	// crop or apply a ramp function to splat
	// information outside of unit circle will not be displayed properly
	//fimage_circle_crop( &splat );
	fimage_circle_ramp( &splat );
	
	//int nruns = 1000;
	int nruns = 200;
	cluster runs[ nruns ];
	vfield vf, vf_vanish, vf_perturb, auroracentric;
	vect2 start; v_set( &start, 7.5, 7.5 );

	//v_set( &vbound1, -20.0, -20.0 * (1.0 * base.ydim) / (1.0 * base.xdim) );
	//v_set( &vbound2,  30.0,  30.0 * (1.0 * base.ydim) / (1.0 * base.xdim) );
	v_set( &vbound1,  -5.0, -5.0 );
	v_set( &vbound2,  15.0, 20.0 );

	aurora_palette( &color_palette ); // set palette 

	vfield_initialize( 1000, abs( (int)(1000 * (vbound2.y - vbound1.y) / (vbound2.x - vbound1.x) ) ), vbound1, vbound2, &vf);
	vfield_initialize( 1000, abs( (int)(1000 * (vbound2.y - vbound1.y) / (vbound2.x - vbound1.x) ) ), vbound1, vbound2, &vf_vanish);
	//vfield_initialize( 1000, abs( (int)(1000 * (vbound2.y - vbound1.y) / (vbound2.x - vbound1.x) ) ), vbound1, vbound2, &vf_perturb);
	//vfield_turbulent( 2.0, 250, false, &vf_perturb );
	//vfield_normalize(  &vf_perturb );
	//vfield_scale( 0.5, &vf );

	vect2 vanishing_point; 	v_set( &vanishing_point, 5.0, 20.0 );
	vect2 aurorigin;		v_set( &aurorigin, 5.0, 7.5);

	// Create concentric vector field for aurora beams with center above top of frame
	vfield_concentric( vanishing_point, &vf_vanish );
	vfield_scale( -1.0, &vf_vanish );
	vfield_normalize( &vf_vanish );

	vfield_concentric( aurorigin, &vf );
	vfield_normalize( &vf );
	//vfield_sum( &vf, &auroracentric, &vf );
	//vfield_normalize( &vf );

	vortex_field.n = 250;
	vortex_field.vortex_list = (vortex *)malloc( vortex_field.n * sizeof( vortex ) );
	vortex *vort = vortex_field.vortex_list;
	if( debug_me ) printf(" vortex_field.n = %d\n", vortex_field.n );
	for( i = 0; i < vortex_field.n; i++ ) {
		if( rand()%2 ) 	vort->speed_of_rotation = 1.0;
			else 		vort->speed_of_rotation = -1.0;
		vort->diameter = 2.0;
		vort->soften = 0.5;
		vort->center_of_revolution = box_of_random2( base.min, base.max );
		vort->start_offset.x = rand1() * 5.0; 	vort->start_offset.y = 0.0;
		vort->start_offset = v_rotate( vort->start_offset, rand1() * 360.0 );
		if( rand()%2 ) 	vort->revolution_velocity = 1.0;	// Should be multiple of 1.0
			else 		vort->revolution_velocity = -1.0;
		vort++;
	}

	int frame, aframe;
	float ang = 0.0;
	float ang_inc = 2.0;
	float start_step = 15.0 / nframes;

	int n_aurorae = 20;
	vect2 starts[ n_aurorae ];
	int frame_offset[ n_aurorae ];
	int ang_offset[ n_aurorae ];
	float start_angle;
	vect2 start_offset; 
	wave_params ang_wave;
	make_random_wave( 	4, 										// number of composed waves
						75.0,									// max magnitude
						6, 										// max run frequency
						5, 										// max frame frequency
						&ang_wave ); 							// pointer to wave param 

	float aurotor_speed = 0.0;
	for( frame = 0; frame < nframes; frame++ ) {
		for( a = 0; a < n_aurorae; a++ ) {

			start_angle = (a + aurotor_speed * frame / nframes) * 360.0 / n_aurorae;
			v_set( &start_offset, 3.0, 0.0 );
			start_offset = v_rotate( start_offset, start_angle );
			start = v_add( start_offset, aurorigin );

			generate_aurora( 	1.0 * frame / nframes,
								nruns, 			// Number of sub-streams in aurora
								bound1, 		// Bounds of image
								bound2, 
								start, 			// starting point
								0.0,	// offset angle
								&ang_wave,	// angle wave parameters
								&color_palette,
								&vf, 
								&vortex_field,	// perturbation field parameters
								runs, 
								&scn);		// pass pointer to scene to be added to cluster
			render_cluster_list( nruns, ang, runs, &vf_vanish, &splat, render_to, &scn );
		}

		printf("frame = %d\n",frame);

		// Clip colors in image to prevent excessive darkening
		fimage_clip( 0.0, 1.0, render_to );
		//fimage_normalize( render_to );

		if( scn.reflect_y ) fimage_reflect_y( scn.reflect_y_line, true, render_to, &result);	// reflect image if required
		if( scn.use_mask ) fimage_apply_mask( &base, &mask, &result );
		//fimage_normalize( &result );

		// save result
		sprintf( output_filename, "%s_%04d.jpg", scn.output_basename, frame);
		fimage_write_jpg( output_filename, &result );

		if( scn.use_mask ) fimage_copy_contents( &background, &background_buffer );
		else fimage_copy_contents( &base, &result );
	}

	/*
	for( a=0; a < n_aurorae; a++ )	{
		free_wave_params( &(ang_wave[ a ]) );
		free_wave_params( &(brightness_wave[ a ]) );
	} */

	free_wave_params( &ang_wave );
	free_vect_fn_2d_t_params( &vortex_field );

	fimage_free( &base );
	fimage_free( &splat );
	fimage_free( &result );	
	if( scn.use_mask ) {
		fimage_free( &background );
		fimage_free( &mask );
		fimage_free( &background_buffer );
	}
	if( scn.use_stencil ) {
		fimage_free( &stencil );
	}
	return 0;
}
