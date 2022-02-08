#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "vect2.h"
#include "vfield.h"

// ********************** Constructor / destructor ********************** 

// Set size and allocate memory for vector field
void vfield_initialize( int xsiz, int ysiz, vect2 fmin, vect2 fmax, vfield *vf )
{
	vf->xdim = xsiz;
	vf->ydim = ysiz;
	vf->min = fmin;
	vf->max = fmax;
	vf->f = (vect2 *)malloc( xsiz * ysiz * sizeof(vect2) );
}

// free memory associated with vector field
void vfield_free( vfield *f )
{
	free(f->f);
}

// ********************** Access functions ********************** 

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

// ********************** Modification functions ********************** 

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

// ********************** Combination functions ********************** 

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

// ********************** Generation functions ********************** 

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
