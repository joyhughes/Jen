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
	float x,y;
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

	// vector field associated with cluster 
	// vfield *field;

	// Size properties
	float size;		// Size of object in parametric space
	bool size_prop;		// Is size proportional to step size?

	// Step properties
	float step;		// Initial step size
	float prop;		// Proportional change in stepsize per step
	bool bounded;		// Is there a boundary condition?
	vect2 bmin,bmax;	// Bounding rectangle for cluster

	// Move at an angle to vector field
	float ang_offset; // Angle of motion relative to vector field
	// float ang_offset_inc;  
	// other angle offset parameters - proportional to size?

	// Parameters for handling angles (in degrees)
	float ang_init;	// initial angle
	float ang_inc;		// angle increment
	bool   ang_relative;	// Are angles relative to vector direction?

	// Color properties
	bool color_filter;	// Filter the splat color
	float start_color;	// starting color in rainbow function
	float color_inc;   // color change
	float brightness;	// overall brightness of splat
	float brightness_prop; // proportional change in brightness per step
	// other brightness change properties here

	// animation properties
	// ...

	// branching properties - spawn new clusters recursively

} cluster;

typedef struct FRGB
{
	float r,g,b;
} frgb;

typedef struct FIMAGE
{
	int xdim, ydim;	
	vect2 min, max;		// Bounds of image in cartesian space
	frgb *f;			// Pointer to array of frgb pixels
} fimage;

// returns a random number between 0 and 1
float rand1()
{
	return (rand() * 1.0) / (RAND_MAX * 1.0);
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

// Initialize cluster with plausible default properties
void c_initialize( cluster *k )
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

	// Splat orientation
	k->ang_init = 0.0;
	k->ang_inc = 0.0;
	k->ang_relative = true;	// We like the relative angle

	//Color Filter
	k->color_filter = false;
	k->start_color = 0.0;
	k->color_inc = 0.0;
	k->brightness = 1.0;
	k->brightness_prop = 1.0;
}

void c_set_run( int n, vect2 start, float step, float prop, float ang_offset, cluster *uck )
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

void c_set_color( float start_color, float color_inc, float brightness, float brightness_prop, cluster *uck )
{
	uck->color_filter = true;
	uck->start_color = start_color;
	uck->color_inc = color_inc;
	uck->brightness = brightness;
	uck->brightness_prop = brightness_prop;
}

void c_set_size( bool size_prop, float size, cluster *uck )
{
	uck->size_prop = size_prop;
	uck->size = size;
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

float add_angle( float a1, float a2 )
{
	float a = fmod(a1 + a2, 360.0 );
	if( a < 0.0 ) a += 360.0;
	return a;
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

// Returns true if vector v is within bounding box defined by min and max
bool in_bounds( vect2 v, vect2 min, vect2 max)
{
	bool inx,iny;

	if( min.x < max.x) {
		inx = ( v.x >= min.x ) &&  ( v.x <= max.x );
	}
	else {
		inx = ( v.x >= max.x ) &&  ( v.x <= min.x );
	}

	if( min.y < max.y) {
		iny = ( v.y >= min.y ) &&  ( v.y <= max.y );
	}
	else {
		iny = ( v.y >= max.y ) &&  ( v.y <= min.y );
	}

	return( inx && iny);
}


// Set size and allocate memory for vector field
void vfield_initialize( int xsiz, int ysiz, vect2 fmin, vect2 fmax, vfield *f )
{
	f->xdim = xsiz;
	f->ydim = ysiz;
	f->min = fmin;
	f->max = fmax;
	f->f = (vect2 *)malloc( xsiz * ysiz * sizeof(vect2) );
}

// Returns the value of the vector field given integer indices
vect2 vfield_index( int xi, int yi, vfield *f )
{
	return( f->f[ (xi % f->xdim) + f->xdim * (yi % f->ydim)]);
}

// Returns coordinate in vector field's linear space given integer indices
vect2 vfield_coord( int xi, int yi, vfield *f )
{
	vect2 w;

	w.x = xi * ( f->max.x - f->min.x ) / ( 1.0 * f->xdim ) + f->min.x;
	w.y = yi * ( f->max.y - f->min.y ) / ( 1.0 * f->ydim ) + f->min.y;

	return w;
}

// Returns interpolated value of vector field given floating point indices
vect2 vfield_smooth_index( vect2 v, vfield *f )
{
	vect2 w,u00,u01,u10,u11,u;
	int xi, yi;		// Integer part of index
	float xf, yf;	// Fractional part of index

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
	u00 = vfield_index( xi,   yi,   f );
	u01 = vfield_index( xi,   yi+1, f );
	u10 = vfield_index( xi+1, yi,   f );
	u11 = vfield_index( xi+1, yi+1, f );

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
			if( in_bounds( vfield_coord( xi, yi, f ), min, max ) )
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
void vfield_turbulent( float diameter, float n, vfield *f)
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
		if( rand()%2 ) vfield_scale( -2.0, &g );		// coin flip for rotational direction
		
		vfield_inverse_square( diameter, 0.5, &g );
		vfield_sum( f, &g, f );
	}
	vfield_free( &g );		// free memory used in buffer
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

// presume here f is already initialized - that way we can reuse buffer
void quantize( unsigned char *img, fimage *f )
{
	int x, y;
	unsigned char *c;
	frgb *p = f->f;
	int xdim = f->xdim;
	int ydim = f->ydim;

	img = (unsigned char *)malloc( 3 * f->xdim * f->ydim );
	if( img == NULL ) printf("Quantize allocation error\n");
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

void fimage_copy( fimage *in, fimage *out)
{
	int x,y;
	frgb *pin = in->f;
	frgb *pout = out->f;
	int xdim = in->xdim;
	int ydim = in->ydim;

	for( y = 0; y < ydim; y++ ) {
		for( x = 0; x < xdim; x++ ) {
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

void fimage_filter( frgb color, fimage *f )
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
void fimage_rotate( float theta, fimage *in, fimage *out )
{
	int x, y, ox, oy;
	float fx, fy, fox, foy;
	frgb *o = out->f;
	int xdim = in->xdim;
	int ydim = in->ydim;
	float half_xdim = xdim / 2.0;
	float half_ydim = ydim / 2.0;

	float cth = cos( -theta );	// ulhu
	float sth = sin( -theta );

	for( y = 0; y < ydim; y++ )
	{
		for( x = 0; x < xdim; x++ )
		{
			fx = x * 1.0 - half_xdim;
			fy = y * 1.0 - half_ydim;
			fox = fx * cth - fy * sth;
			foy = fx * sth + fy * cth;
			ox = (int)(fox + half_xdim);
			oy = (int)(foy + half_ydim);

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
// this function can adjust number of subjects in run if it wants to
// right now just prints but can add this to a structure if need be
// occasional mutation changes single member of run or remainder of run
void render_cluster( vfield *f, cluster *uck, fimage *result, fimage *splat )
{
	vect2 w = uck->start;
	vect2 u;
	float step = uck->step;
	float ang = uck->ang_init;
	float rel_ang;
	int i=0;
	frgb tint;
	float rainbow_index = uck->start_color;
	float size = uck->size;
	float brightness = uck->brightness;

	if( !uck->color_filter ) {
		tint.r = 1.0; tint.g = 1.0; tint.b = 1.0;
	}

	while( i < uck->n )
	{
		// calculate angle
		rel_ang = ang;
		if( uck->ang_relative )
		{
			u = vfield_smooth_index( w, f );
			v_rotate( u, uck->ang_offset );
			u.x *= step; u.y *= step;
			rel_ang = add_angle( rel_ang, vtoa( u ) );
		}

		if( uck->color_filter ) {
			tint = rainbow( rainbow_index );
			tint.r *= brightness;
			tint.g *= brightness;
			tint.b *= brightness;
			brightness *= uck->brightness_prop;
		}
		if( uck->size_prop ) size = step * uck->size;

		if(uck->bounded)	// Truncate cluster if out of bounds
		{
			if( in_bounds( w, uck->bmin, uck->bmax ) ) 
			{
				// *** generate here ***
				fimage_splat( 
					w, 				// coordinates of splat center
					size, 			// radius of splat
					rel_ang, 		// rotation in degrees
					tint,			// change the color of splat
					result, 		// image to be splatted upon
					splat 			// image of the splat
				);
			}
		}

		rainbow_index += uck->color_inc;
		w = vfield_advect( w, f, step, uck->ang_offset );	// move through vector field associated with subspace
		ang = add_angle( ang, uck->ang_inc );
		step *= uck->prop;

		// blend through property (size color etc)
		
		i++;
	}
}

void render_cluster_list( int nruns, cluster *clist, vfield *vf, fimage *splat, fimage *result)
{
	cluster *uck = clist;
	int run;

	for(run = 0; run < nruns; run++ ) {
		render_cluster( vf, uck, result, splat );
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
// generate_line()

// creates a random distribution of runs
void generate_random( int nruns, vect2 imin, vect2 imax, float ang_offset, cluster *clist)
{
	cluster *uck = clist;
	int run;
	vect2 start;
	vect2 vmin, vmax;	// bounds of vector field

	vmin.x = -1.0;	vmin.y = -1.0;
	vmin.x = 11.0;	vmin.y = 11.0;

	for(run = 0; run < nruns; run++ ) {
		start = box_of_random2( vmin, vmax );
		//printf("start.x = %f start.y = %f\n", start.x, start.y);
		c_initialize( uck );

		c_set_run( 	200, 		// number of elements in run
					start,		// starting point of run 
					0.0625, 	// initial step size
					0.99, 		// proportional change per step
					ang_offset, // motion relative to vector field
					uck );		// pointer to cluster

		c_set_bounds( imin, imax, uck );

		c_set_color( 	rand1() * 0.33 + 0.33, 		// Initial color
						rand1() * 0.005 - 0.0025, 		// Color increment
						0.5, 		// Initial brightness
						1.0, 		// Brightness proportional change per step
						uck );

		c_set_size( true, 2.0, uck );

		uck++;
	}
}

// The "jaggie", a colored line with (somewhat) sharp angles
void generate_jaggie( fimage *result, fimage *splat )
{
	vfield f;		// Main vector field for iterating
	vect2 d, c, start, min, max;		// flow direction
	time_t t;
	cluster k;
	int i;
	
	// start random engines
	srand((unsigned) time(&t));

	// set bounds of vector field larger than image
	min.x =  -1.0; min.y =  -1.0;
	max.x =  11.0; max.y =  11.0;
	vfield_initialize( 1000, 1000, min, max, &f );

	d.x = 1.0; d.y = 0.0;
	vfield_linear( d, &f );
	vfield_add_stripes( &f );
	vfield_normalize( &f );
	start.x = 0.0625; start.y = 4.0;
	c_initialize( &k );

	c_set_run( 	800, 			// Number of subjects in run
			start, 				// Starting point
			0.0625, 			// Initial step size
			1.0, 				// Step size proportional change per step
			0.0,				// No adjustment to angle
			&k );	

	c_set_bounds( f.min, f.max, &k );

	c_set_color( 	0.0, 		// Initial color
					0.005, 		// Color increment
					0.5, 		// Initial brightness
					1.0, 		// Brightness proportional change per step
					&k );

	c_set_size( true, 2.0, &k  );
	render_cluster( &f, &k, result, splat );
}

// Sanity check to see if splat is working
// needs to include tint
void test_splat( fimage *r, fimage *f )
{
	// splat it
	vect2 center;
	center.x = 5.0;
	center.y = 5.0;
	//fimage_splat( center, 1.0, 30.0, &r, &f );
}

int main( int argc, char const *argv[] )
{
    int xdim, ydim, s_xdim, s_ydim, channels;
    frgb black;
    fimage base, splat, result, mask;
    char *basename;
    char *filename;
    unsigned char *img;
    int i;
    vect2 bound1,bound2;

    black.r = 0.0;	black.g = 0.0;	black.b = 0.0;

	// load base image
	if( argc >= 4 ) 
	{
      img = stbi_load( argv[1], &xdim, &ydim, &channels, 0 );
      if(img == NULL) 
      {
         printf("Error in loading base image\n");
         return 0;
      }
  	}
  	else
   	{ 
      printf("Usage: ./lux base splat mask\n");
      return 0; 
   	}
	   
	basename = remove_ext( (char *)argv[1], '.', '/' );   // scan input filename for "." and strip off extension, put that in basename
	filename = (char*)malloc( strlen(basename) + 12 );    // allocate output filename with room for code and extension
	  
	// initialize base image
	// square images 10 x 10 work better with current vector fields
	// origin in lower left
	bound1.x = 0.0;
	bound1.y = 10.0 * (1.0 * ydim) / (1.0 * xdim);
	bound2.x = 10.0;
	bound2.y = 0.0;
	fimage_init( xdim, ydim, bound1, bound2, &base );

	// convert base image to frgb
	continuous( channels, img, &base );
	free( img );

	// Initialize result image
	fimage_init( xdim, ydim, bound1, bound2, &result );
	fimage_copy( &base, &result );
	//fimage_make_mask( 0.1, &base, &result );

	// load splat image
	img = stbi_load( argv[2], &s_xdim, &s_ydim, &channels, 0 );
    if(img == NULL) 
    {
       printf("Error in loading splat image\n");
       return 0;
    }

	bound1.x = -1.0;
	bound1.y =  1.0;
	bound2.x =  1.0;
	bound2.y = -1.0;

	// Splat image assumed to be square, otherwise it will be squeezed
	fimage_init( s_xdim, s_ydim, bound1, bound2, &splat );	
	continuous( channels, img, &splat );
	free( img );

	// crop or apply a ramp function to splat
	// information outside of unit circle will not be displayed properly
	//fimage_cirle_crop( &f );
	fimage_circle_ramp( &splat );

	// load mask image - must be same dimensions as base image
	img = stbi_load( argv[3], &xdim, &ydim, &channels, 0 );
    if(img == NULL) 
    {
       printf("Error in loading mask image\n");
       return 0;
    }

	// initialize mask image
	bound1.x = 0.0;
	bound1.y = 10.0 * (1.0 * ydim) / (1.0 * xdim);
	bound2.x = 10.0;
	bound2.y = 0.0;
	fimage_init( xdim, ydim, bound1, bound2, &mask );

	// convert mask image to frgb
	continuous( channels, img, &mask );
	free( img );

	// generate_jaggie( &result, &splat );
	
	int nruns = 150;
	cluster runs[ nruns ];
	// future: a structure that holds parameters for randomness (loaded from file)
	generate_random( nruns, bound1, bound2, 90.0, runs );
	vfield vf;

	bound1.x = -1.0;
	bound1.y = -1.0;
	bound2.x = 11.0;
	bound2.y = 11.0;

	vfield_initialize( 1000, 1000, bound1, bound2, &vf);
	vfield_turbulent( 7.5, 10, &vf );
	vfield_normalize( &vf );

	// separated generation from rendering
	render_cluster_list( nruns, runs, &vf, &splat, &result);

	// Clip colors in image to prevent excessive darkening
	fimage_clip( 0.0, 1.5, &result );
	fimage_normalize( &result );
	// add base image back in
	fimage_sum( &base, &result, &result );	
	fimage_apply_mask( &base, &mask, &result );

	// normalize and render
	fimage_normalize( &result );

	// save result
	quantize( img, &result );
	sprintf( filename, "%s_splat.jpg", basename);
	stbi_write_jpg(filename, xdim, ydim, 3, img, 100);

	free( img );
	fimage_free( &base );
	fimage_free( &splat );
	fimage_free( &result );	
	fimage_free( &mask );
	return 0;
}
