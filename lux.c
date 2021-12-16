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

typedef struct Vect2 
{
	double x,y;
} vect2;

typedef struct Vfield
{
	int xdim, ydim;		// Size of image in pixels
	vect2 min, max;		// Bounds of field in cartesian space
	vect2 *f;			// Flow vectors
} vfield;

// Specifies a grouping of subjects with various generative rules
// Avoids functions with a bazillion arguments
typedef struct Cluster
{
	// element *id;		// What am I?
	int n;				// Number of subjects in cluster
	vect2 start;		// Location of first item in cluster

	// Step properties
	double step;		// Initial step size
	double prop;		// Proportional change in stepsize per step
	bool bounded;	// Is there a boundary condition?
	vect2 bmin,bmax;	// Bounding rectangle for cluster
	// Move at an angle to vector field
	double ang_offset; // Angle of motion relative to vector field

	// Parameters for handling angles (in degrees)
	double ang_init;	// initial angle
	double ang_inc;		// angle increment
	bool ang_relative;	// Are angles relative to vector direction?

} cluster;

typedef struct FRGB
{
	double r,g,b;
} frgb;

typedef struct FIMAGE
{
	int xdim, ydim;	
	vect2 min, max;		// Bounds of image in cartesian space
	frgb *f;			// Pointer to array of frgb pixels
} fimage;

// returns a random number between 0 and 1
double rand1()
{
	return (rand() * 1.0) / (RAND_MAX * 1.0);
}

// rotate a vector by theta degrees
vect2 v_rotate( vect2 in, double theta)
{
	vect2 out;
	double thrad = theta / 360.0 * TAU;

	out.x = in.x * cos( thrad ) - in.y * sin( thrad );
	out.y = in.x * sin( thrad ) + in.y * cos( thrad );

	return out;
}

void c_set_run( int n, vect2 start, double step, double prop, double ang_offset, cluster *uck )
{
	uck->n = n;
	uck->start = start;
	uck->step = step;
	uck->prop = prop;
	uck->ang_offset = ang_offset;
}

void c_set_bounds( vect2 bmin, vect2 bmax, cluster *uck )
{
	uck->bounded = true;
	uck->bmin = bmin;
	uck->bmax = bmax;
}

double v_magnitude( vect2 v )
{
	return(sqrt(v.x*v.x + v.y*v.y));
}

// Returns angle corresponding to vector (in degrees)
// Wondering if I should correspond to NESW?
double vtoa( vect2 v )
{
	double ang;

	ang = atan2( v.y, v.x ) / TAU * 360.0;
	if( ang < 0.0 ) ang += 360.0;
	return ang;
}

double add_angle( double a1, double a2 )
{
	double a = fmod(a1 + a2, 360.0 );
	if( a < 0.0 ) a += 360.0;
	return a;
}

vect2 v_normalize( vect2 v )
{
	vect2 w;
	double m = v_magnitude(v);
	
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


// Chooses a random point within 3D box - assumes srand() has already been called
vect2 box_of_random2( vect2 min, vect2 max)
{
	vect2 w;
	double xprop = rand1();
	double yprop = rand1();

	w.x = min.x * (1.0 - xprop) + max.x * xprop;
	w.y = min.y * (1.0 - yprop) + max.y * yprop;

	return w;
}

// Returns true if vector v is within bounding box defined by min and max
bool in_bounds( vect2 v, vect2 min, vect2 max)
{
	return( ( v.x >= min.x ) &&  ( v.y >= min.y ) && ( v.x <= max.x ) && ( v.y <= max.y ) );
}

// Returns the value of the vector field given integer indices
vect2 f_index( int xi, int yi, vfield *f )
{
	return( f->f[ (xi % f->xdim) + f->xdim * (yi % f->ydim)]);
}

// Returns coordinate in vector field's linear space given integer indices
vect2 f_coord( int xi, int yi, vfield *f )
{
	vect2 w;

	w.x = xi * ( f->max.x - f->min.x ) / ( 1.0 * f->xdim ) + f->min.x;
	w.y = yi * ( f->max.y - f->min.y ) / ( 1.0 * f->ydim ) + f->min.y;

	return w;
}

// Returns interpolated value of vector field given floating point indices
vect2 smooth_index( vect2 v, vfield *f )
{
	vect2 w,u00,u01,u10,u11,u;
	int xi, yi;		// Integer part of index
	double xf, yf;	// Fractional part of index

	// calculate index of vector in rational space
	// out of bounds vectors just wrap around - index will take the modulus
	w.x = (v.x - f->min.x) / (f->max.x - f->min.x) * f->xdim;
	w.y = (v.y - f->min.y) / (f->max.y - f->min.y) * f->ydim;

	// locate bounding cell of vector 
	xi = floor(w.x);
	xf = w.x - xi * 1.0;
	yi = floor(w.y);
	yf = w.y - yi * 1.0;

	// linearly interpolate beween surrounding corners
	u00 = f_index( xi,   yi,   f );
	u01 = f_index( xi,   yi+1, f );
	u10 = f_index( xi+1, yi,   f );
	u11 = f_index( xi+1, yi+1, f );

	u.x = (u00.x * ( 1.0 - xf ) + u10.x * xf) * (1.0 - yf) + (u01.x * ( 1.0 - xf ) + u11.x * xf) * yf;
	u.y = (u00.y * ( 1.0 - xf ) + u10.y * xf) * (1.0 - yf) + (u01.y * ( 1.0 - xf ) + u11.y * xf) * yf;

	return u;
}

// Use Newton's method to move along flow line proportional to step value
vect2 advect( vect2 v, vfield *f, double step, double angle)
{
	vect2 w,u;

	w = smooth_index( v, f );
	v_rotate( w, angle );
	u.x = v.x + step * w.x;
	u.y = v.y + step * w.y;

	return u;
}

// Set size and allocate memory for vector field
void f_initialize( int xsiz, int ysiz, vect2 fmin, vect2 fmax, vfield *f )
{
	f->xdim = xsiz;
	f->ydim = ysiz;
	f->min = fmin;
	f->max = fmax;
	f->f = (vect2 *)malloc( xsiz * ysiz * sizeof(vect2) );
}

// free memory associated with vector field
void f_free( vfield *f )
{
	free(f->f);
}

// Return a vector field with each vector 90 degrees to the left
void f_complement( vfield *f)
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
void f_scale( double s, vfield *f)
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

// Return sum of two vector fields
// assume all of same dimension
void f_sum( vfield *a, vfield *b, vfield *f)
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

// Creates an inverse square of vector field with optional softening factor to avoid infinity 
void f_inverse_square( double diameter, double soften, vfield *f )
{
	int xi, yi;
	vect2 *w = f->f;
	vect2 u;
	double s;

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
void f_normalize( vfield *f )
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
void f_linear( vect2 v, vfield *f)
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
void f_concentric( vect2 center, vfield *f )
{
	int xi, yi;
	vect2 *w = f->f;
	vect2 u;

	for(yi = 0; yi < f->ydim; yi++ )
	{
		for(xi = 0; xi < f->xdim; xi++ )
		{
			u = f_coord( xi, yi, f );
			w->x = u.x - center.x;
			w->y = u.y - center.y;
			w++;
		}
	}
}

// creates an array of vectors rotating around center
void f_rotation( vect2 center, vfield *f )
{
	// rotation field is complement of concentric field
	f_concentric( center, f );
	f_complement( f );
}

// creates a spiraling vector field by combining concentric and rotational fields in proportion
void f_spiral( vect2 center, double cscale, double rscale, vfield *f )
{
	vfield g;
	f_initialize( f->xdim, f->ydim, f->min, f->max, &g );	// initialize buffer

	f_concentric( center, f );
	f_scale( cscale, f );
	f_rotation( center, &g );
	f_scale( rscale, &g );
	f_sum( f, &g, f );

	f_free( &g );		// free memory used in buffer
}

// Adds a constant vector over box shaped area into a vector field
void f_box( vect2 min, vect2 max, vect2 v, vfield *f )
{
	int xi, yi;
	vect2 *w = f->f;

	for(yi = 0; yi < f->ydim; yi++ )
	{
		for(xi = 0; xi < f->xdim; xi++ )
		{
			if( in_bounds( f_coord( xi, yi, f ), min, max ) )
			{
				w->x += v.x;
				w->y += v.y;
			}
			w++;
		}
	}
}

void f_add_stripes( vfield *f )
{
	int n=4;	// four jags /* plus one to rule them all */
	int i;
	double w;	// width of stripe
	double stripex;	// x position of stripe
	vect2 v, min, max;
	double c, vv;

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
		printf(" stripe %d  w = %.2f  stripex = %.2f  v.y = %.2f\n", i, w, stripex, v.y );
		f_box( min, max, v, f );
		min.x = stripex;
		max.x = stripex + w;
		v.y = -1.0 * v.y;
		f_box( min, max, v, f );
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
}

// Composite vector field including multiple centers of rotation
void f_turbulent( double diameter, double n, vfield *f)
{
	int i;
	vect2 w,c;
	vfield g;	// buffer to hold each center of rotation before adding in

	// Set value of output array to zero
	w.x= 0.0; w.y=0.0;
	f_linear(w,f);

	f_initialize( f->xdim, f->ydim, f->min, f->max, &g );	// initialize buffer
	for(i=0; i<n; i++)
	{
		c = box_of_random2( g.min, g.max );
		f_rotation( c, &g );
		if( rand()%2 ) f_scale( -2.0, &g );		// coin flip for rotational direction
		
		f_inverse_square( diameter, 0.5, &g );
		f_sum( f, &g, f );
	}
	f_free( &g );		// free memory used in buffer
}

// "runs" are clusters of subjects advected through vector field
// run can continue a certain number of iterations or until crossing boundary condition
// this function can adjust number of subjects in run if it wants to
// right now just prints but can add this to a structure if need be
// occasional mutation changes single member of run or remainder of run
void generate_cluster( cluster *uck, vfield *f)
{
	vect2 w = uck->start;
	vect2 u;
	double step = uck->step;
	double ang = uck->ang_init;
	double rel_ang;
	int i=0;

	while( i < uck->n )
	{
		// calculate angle
		rel_ang = ang;
		if( uck->ang_relative )
		{
			u = smooth_index( w, f );
			v_rotate( u, uck->ang_offset );
			u.x *= step; u.y *= step;
			rel_ang = add_angle( rel_ang, vtoa( u ) );
		}
		// generate here - for now just print for sanity check
		printf( "subj %d\n",i);
		//screen_pixel( w, f );
		//nearest_16th( w.x );
		//printf( "\n  y = ");
		//nearest_16th( w.y );
		printf( "  angle = %.0f degrees \n", rel_ang );
		//printf( "  step = %.2f   ", step );
		// printf( "  prop = %f", k->prop);
		//nearest_16th( step );
		printf("\n\n");
		w = advect( w, f, step, uck->ang_offset );	// move through vector field associated with subspace
		ang = add_angle( ang, uck->ang_inc );
		step *= uck->prop;
		if(uck->bounded)	// Truncate cluster if out of bounds
		{
			if( !in_bounds( w, uck->bmin, uck->bmax ) ) 
			{
				uck->n = i;
			}
		} 
		// blend through property (size color etc)
		
		i++;
	}
}

// creates a grouping of runs evenly spaced in a circle
// occasional mutation changing some aspect of a run
void generate_circle( 	vect2 c, 	// center of circle
						double r, 	// radius of circle
						double start_ang,	// starting angle in degrees
						int m,			// number of runs around circle
						cluster *uck,
						vfield *f )
{
	int i;
	double theta = TAU * start_ang / 360.0;

	for(i=0; i<m; i++)
	{
		uck->start.x = c.x + r * cos(theta);
		uck->start.y = c.y + r * sin(theta);
		generate_cluster( uck, f );
		theta += TAU / m;
	}
}

// generates a cluster in a regular grid, aligned along vector field
void generate_grid( vect2 min, vect2 max, int xsteps, int ysteps, double mutation_rate, vfield *f )
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
			w = smooth_index( v, f );
			printf("  w.x = %.2f  w.y = %.2f\n",w.x,w.y);
			printf("  angle = %.0f\n\n", vtoa( w ) );
		}
	}
}

// creates a grouping of runs in a line
// generate_line()

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

frgb rainbow( double a )
{
	frgb c;

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

void fimage_init( long xdim, long ydim, vect2 min, vect2 max, fimage *f )
{
	f->xdim = xdim;
	f->ydim = ydim;
	f->min = min;
	f->max = max;
	f->f = (frgb *)malloc( xdim * ydim * sizeof(frgb) );
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
void continuous( int channels, unsigned char *img, fimage *f )
{
	int x, y;
	unsigned char *c = img;
	frgb *p = f->f;

	for( y = 0; y<f->ydim; y++ )
	{
		for( x = 0; x<f->xdim; x++ )
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

// presume here f is already initialized - that way we can reuse buffer
void quantize( unsigned char *img, fimage *f )
{
	int x, y;
	unsigned char *c;
	frgb *p = f->f;

	img = (unsigned char *)malloc( 3 * f->xdim * f->ydim );
	if( img == NULL ) printf("Quantize allocation error\n");
	c = img;

	for( y = 0; y<f->ydim; y++ )
	{
		for( x = 0; x<f->xdim; x++ )
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

void fimage_copy( fimage *in, fimage *out)
{
	int x,y;
	frgb *pin = in->f;
	frgb *pout = out->f;

	for( y = 0; y<in->ydim; y++ ) {
		for( x = 0; x<in->xdim; x++ ) {
			*pout = *pin;
			pin++;
			pout++;
		}
	}
}

// assume all images are the same size
void fimage_sum( fimage *a, fimage *b, fimage *result )
{
	int x,y;
	frgb *pa = a->f;
	frgb *pb = b->f;
	frgb *pr = result->f;

	for( y = 0; y<a->ydim; y++)	
	{
		for( x = 0; x<a->xdim; x++)
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

	for( y = 0; y<a->ydim; y++)	
	{
		for( x = 0; x<a->xdim; x++)
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

	for( y = 0; y<f->ydim; y++)	
	{
		for( x = 0; x<f->xdim; x++)
		{
			p->r = color.r;
			p->g = color.g;
			p->b = color.b;

			p++;
		}
	}
}

void fimage_filter( frgb color, fimage *f )
{
	int x,y;
	frgb *p = f->f;

	for( y = 0; y<f->ydim; y++)	
	{
		for( x = 0; x<f->xdim; x++)
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

	for( y = 0; y<f->ydim; y++)	
	{
		for( x = 0; x<f->xdim; x++)
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
	double max = 0.0;

	// scan for maximum value
	for( y = 0; y < f->ydim; y++ )	
	{
		for( x = 0; x < f->xdim; x++ )
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
		for( y = 0; y < f->ydim; y++ )	
		{
			for( x = 0; x < f->xdim; x++) 
			{
				p->r /= max;
				p->g /= max;
				p->b /= max;

				p++;
			}
		}
	}
}

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

// in ang out images must be separate
void fimage_rotate( double theta, fimage *in, fimage *out )
{
	int x, y, ox, oy;
	double fx, fy, fox, foy;
	frgb *o = out->f;

	double cth = cos( -theta );	// ulhu
	double sth = sin( -theta );

	for( y=0; y<in->ydim; y++ )
	{
		for( x=0; x<in->xdim; x++ )
		{
			fx = x * 1.0 - in->xdim / 2.0;
			fy = y * 1.0 - in->ydim / 2.0;
			fox = fx * cth - fy * sth;
			foy = fx * sth + fy * cth;
			ox = (int)(fox + in->xdim / 2.0);
			oy = (int)(foy + in->ydim / 2.0);

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

	for( y=0; y<in->ydim; y++ )
	{
		for( x=0; x<in->xdim; x++ )
		{
			ox = x - xoff;
			oy = y - yoff;
			*o = fimage_index( ox, oy, in );
			o++;
		}
	}
}

void fimage_circle_crop( fimage *f )
{	
	double r2;
	frgb *p = f->f;
	int x, y;
	double fx, fy;

	if( f->xdim < f->ydim )
	{
		r2 = f->xdim / 2.0;
		r2 = r2 * r2;
	}
	else
	{
		r2 = f->ydim / 2.0;
		r2 = r2 * r2;		
	}

	for( y = 0; y < f->ydim; y++ )	
	{
		fy = y - f->ydim / 2.0;
		for( x = 0; x < f->xdim; x++ )
		{
			fx = x - f->xdim / 2.0;
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

// Future - include an alpha channel mask in the splat image
// Future - use multiple resolutions for sampling to limit aliasing
void fimage_splat( 
	vect2 center, 			// coordinates of splat center
	double scale, 			// radius of splat
	double theta, 			// rotation in degrees
	fimage *f, 				// image to be splatted upon
	fimage *g 				// image of the splat
	)
{
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
	int scx =  (int)((sc.x * g->xdim / 2.0 + g->xdim / 2.0) * 65536.0);
	int scy =  (int)((sc.y * g->ydim / 2.0 + g->ydim / 2.0) * 65536.0);
	int unxx = (int)(unx.x * g->xdim / 2.0 * 65536.0);
	int unxy = (int)(unx.y * g->xdim / 2.0 * 65536.0);
	int unyx = (int)(uny.x * g->xdim / 2.0 * 65536.0);
	int unyy = (int)(uny.y * g->xdim / 2.0 * 65536.0);
	int sfx, sfy;
	int xdimfix = (g->xdim)<<16;
	int ydimfix = (g->ydim)<<16;

	// *** Critical loop below ***
	// Quick and dirty sampling, high speed but risk of aliasing. 
	// Should work best if splat is fairly large and smooth.

	for( x = xmin; x <= xmax; x++ ) {
		if( (x >= 0) && (x < fxdim) ) {
			sfx = scx;
			sfy = scy;
			for( y = ymin; y <= ymax; y++ ) {
				if( (y >= 0) && (y < fydim) && (sfx >= 0) && (sfy >= 0) && (sfx < xdimfix) && (sfy < ydimfix) ) {
					findex = y * fxdim + x;
					gindex = (sfy >> 16) * gxdim + (sfx >> 16);
					fpix[ findex ].r += gpix[ gindex ].r;
					fpix[ findex ].g += gpix[ gindex ].g;
					fpix[ findex ].b += gpix[ gindex ].b;
				}
				sfx += unyx;
				sfy += unyy;
			}
		}
		scx += unxx;
		scy += unxy;
	}
}

int main( int argc, char const *argv[] )
{
    int xdim, ydim, s_xdim, s_ydim, channels;
    frgb black;
    fimage b, f, r;
    char *basename;
    char *filename;
    unsigned char *img;
    int i;
    vect2 bound1,bound2;

    black.r = 0.0;	black.g = 0.0;	black.b = 0.0;

	// load base image
	if( argc >= 3 ) 
	{
      img = stbi_load( argv[1], &xdim, &ydim, &channels, 0 );
      if(img == NULL) 
      {
         printf("Error in loading first image\n");
         return 0;
      }
  	}
  	else
   	{ 
      printf("./lux base splat\n");
      return 0; 
   	}
	   
	basename = remove_ext( (char *)argv[1], '.', '/' );           // scan input filename for "." and strip off extension, put that in basename
	filename = (char*)malloc( strlen(basename) + 12 );    // allocate output filename with room for code and extension
	  
	// initialize base image
	// square images 10 x 10 work better with current vector fields
	// origin in lower left
	bound1.x = 0.0;
	bound1.y = 10.0 * (1.0 * ydim) / (1.0 * xdim);
	bound2.x = 10.0;
	bound2.y = 0.0;

	// Initialize base image
	fimage_init( xdim, ydim, bound1, bound2, &b );

	// convert base image to frgb
	continuous( channels, img, &b );
	free( img );

	// Initialize sum image
	fimage_init( xdim, ydim, bound1, bound2, &r );
	fimage_copy( &b, &r );

	// load splat image
	img = stbi_load( argv[2], &s_xdim, &s_ydim, &channels, 0 );
    if(img == NULL) 
    {
       printf("Error in loading second image\n");
       return 0;
    }

	bound1.x = -1.0;
	bound1.y =  1.0;
	bound2.x =  1.0;
	bound2.y = -1.0;

	// Splat image assumed to be square, otherwise it will be squeezed
	fimage_init( s_xdim, s_ydim, bound1, bound2, &f );	
	continuous( channels, img, &f );
	free( img );

	
	// splat it
	vect2 center;
	center.x = 5.0;
	center.y = 5.0;
	fimage_splat( center, 1.0, 30.0, &r, &f );

	// normalize and render
	fimage_normalize( &r );

	// save result
	quantize( img, &r );
	sprintf( filename, "%s_splat.jpg", basename);
	stbi_write_jpg(filename, xdim, ydim, 3, img, 100);

	free( img );
	fimage_free( &b );
	fimage_free( &f );
	fimage_free( &r );
	return 0;
}
