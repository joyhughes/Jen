#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

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
	bool comp;		// Move along complement to vector field?
	bool bounded;	// Is there a boundary condition?
	vect2 bmin,bmax;	// Bounding rectangle for cluster
	// Move at an angle to vector field?

	// Parameters for handling angles (in degrees)
	double ang_init;	// initial angle
	double ang_inc;		// angle increment
	bool ang_relative;	// Are angles relative to vector direction?

} cluster;

// returns a random number between 0 and 1
double rand1()
{
	return (rand() * 1.0) / (RAND_MAX * 1.0);
}

// This is what I have to do when the hardware store doesn't sell a metric carpenter's square
// Not sure if this works for negative numbers
void nearest_16th( double r )
{
	int fl, f16;
	bool m = false;

	if( r < 0 )
	{
		m = true;
		r *= -1.0;
	}

	r += ( 1.0 / 32.0 );	// Add 1/32 of an inch for rounding
	fl = floor( r );
	r -= (fl * 1.0);
	f16 = floor( r * 16.0 );

	if( m ) printf("-");
	printf( "%d ",fl);		// Print whole number of inches

	if( f16 != 0 )
	{
		if( !( f16 % 8 ) )
		{
			printf("( 1 / 2 )");
		}
		else if( !( f16 % 4 ) )
		{
			printf("( %d / 4 )", f16 / 4 );
		}
		else if( !( f16 % 2 ) )
		{
			printf("( %d / 8 )", f16 / 2 );
		}
		else printf("( %d / 16 )", f16 );
	}
	printf(" inches ");
}

void screen_pixel( vect2 v, vfield *f )
{
	int xmin = 318;
	int xmax = 834;
	int ymin = 668;
	int ymax = 152;

	int x = (xmax - xmin) * (v.x - f->min.x) / (f->max.x - f->min.x) + xmin; 
	int y = (ymax - ymin) * (v.y - f->min.y) / (f->max.y - f->min.y) + ymin; 

	printf("   x = %d\n",x);
	printf("   y = %d\n",y);
}


// Initialize cluster with plausible default properties
void c_initialize( cluster *k )
{
	k->n = 1;	// Default single subject because that's easier
	k->start.x = 0.0; k->start.y = 0.0;	// Origin
	k->step = 1.0;
	k->prop = 1.0;
	k->comp = false;
	k->bounded = false;					// Use bounding box? Iteration will stop if it leaves bounding box
	k->bmin.x = 0.0; k->bmin.y = 0.0;	
	k->bmax.x = 0.0; k->bmax.y = 0.0;

	k->ang_init = 0.0;
	k->ang_inc = 0.0;
	k->ang_relative = true;	// We like the relative angle
}

void c_set_run( int n, vect2 start, double step, double prop, bool comp, cluster *k )
{
	k->n = n;
	k->start = start;
	k->step = step;
	k->prop = prop;
	k->comp = comp;
}

void c_set_bounds( vect2 bmin, vect2 bmax, cluster *k )
{
	k->bounded = true;
	k->bmin = bmin;
	k->bmax = bmax;
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
vect2 smooth_index( vect2 v, vfield *f, bool comp )
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

	if( comp ) u = v_complement( u );

	return u;
}

// Use Newton's method to move along flow line proportional to step value
vect2 advect( vect2 v, vfield *f, double step, bool comp)
{
	vect2 w,u;

	w = smooth_index( v, f, comp );
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
void generate_cluster( cluster *k, vfield *f)
{
	vect2 w = k->start;
	vect2 u;
	double step = k->step;
	double ang = k->ang_init;
	double rel_ang;
	int i=0;

	while( i<k->n )
	{
		// calculate angle
		rel_ang = ang;
		if( k->ang_relative )
		{
			u = smooth_index( w, f, k->comp );
			u.x *= step; u.y *= step;
			rel_ang = add_angle( rel_ang, vtoa( u ) );
		}
		// generate here - for now just print for sanity check
		printf( "subj %d\n",i);
		screen_pixel( w, f );
		//nearest_16th( w.x );
		//printf( "\n  y = ");
		//nearest_16th( w.y );
		printf( "  angle = %.0f degrees \n", rel_ang );
		//printf( "  step = %.2f   ", step );
		// printf( "  prop = %f", k->prop);
		//nearest_16th( step );
		printf("\n\n");
		w = advect( w, f, step, k->comp );	// move through vector field associated with subspace
		ang = add_angle( ang, k->ang_inc );
		step *= k->prop;
		if(k->bounded)	// Truncate cluster if out of bounds
		{
			if( !in_bounds( w, k->bmin, k->bmax ) ) 
			{
				k->n = i;
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
						cluster *k,
						vfield *f )
{
	int i;
	double theta = TAU * start_ang / 360.0;

	for(i=0; i<m; i++)
	{
		k->start.x = c.x + r * cos(theta);
		k->start.y = c.y + r * sin(theta);
		generate_cluster( k, f );
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
			w = smooth_index( v, f, false );
			printf("  w.x = %.2f  w.y = %.2f\n",w.x,w.y);
			printf("  angle = %.0f\n\n", vtoa( w ) );
		}
	}
}

// creates a grouping of runs in a line
// generate_line()

int main()
{
	vfield f,g;		// Main vector field for iterating, second field for blending
	vect2 d, c, start, min, max;		// flow direction
	time_t t;
	cluster k;
	int i;
	
	// start random engines
	srand((unsigned) time(&t));

	
	min.x =  0.0; min.y =  0.0;
	max.x =  10.0; max.y =  10.0;
	f_initialize( 100, 100, min, max, &f );
	f_initialize( 100, 100, min, max, &g );
	/*
	// for now - test on "scribble"
	// space 30 cm square, origin in center
	d.x = 1.0; d.y = 0.0;
	f_linear( d, &f );
	f_turbulent( 7.5, 10, &g );
	f_scale( 1.35, &g );
	f_sum( &f, &g, &f );
	f_normalize( &f );
	start.x = -14.0; start.y = 0.0;
	generate_run( start, 15, &f );
	*/

	
	d.x = 1.0; d.y = 0.0;
	f_linear( d, &f );
	f_add_stripes( &f );
	f_normalize( &f );
	start.x = 0.0; start.y = 5.0;
	c_initialize( &k );
	c_set_run( 	100, 			// Number of subjects in run
			start, 				// Starting point
			0.5, 				// Initial step size
			1.0, 				// Step size proportional decrease per step
			false,				// Do not follow complementary lines
			&k );	
	c_set_bounds( f.min, f.max, &k );
	generate_cluster( &k, &f );
	
	/*
	// Test 2 - 
	// Space 10 in square, origin in lower left
	c.x = 5.0, c.y = 5.0;
	f_concentric( c, &f );
	f_rotation( c, &g );
	f_sum( &f, &g, &f );
	f_normalize( &f );

	printf("\n\nElement 1\n\n");
	c_initialize( &k );

	c_set_run( 	10, 			// Number of subjects in run
				c, 				// Pick a vector, any vector
				10.0 / 12.0, 	// Initial step size
				0.9, 			// Step size proportional decrease per step
				false,			// Do not follow complementary lines
				&k );		

	generate_circle( 	c, 		// center of circle
						1.0, 	// radius of circle
						0.0,	// start angle
						3,		// number of runs around circle
						&k,
						&f );

	printf("\n\nElement 2\n\n");
	k.comp = true;		// Now move on complementary vector lines
	k.step *= -1.0;		// (Negative complementary vector lines)

	generate_circle( 	c, 		// center of circle
					1.0, 	// radius of circle
					60.0,	// start angle halfway between first ones
					3,		// number of runs around circle
					&k,
					&f );
	*/

/*
	// Test 3 - grid over spiral dipole vector field

	// first center - emitter at 2.5, 7.5
	c.x = 3.5, c.y = 7.0;
	f_spiral( c, 1.0, 1.0, &f );
	f_inverse_square( 3.5, 0.5, &f );

	// second center - sink at 2.5, 7.5
	c.x = 7.0, c.y = 3.5;
	f_spiral( c, -1.0, -1.0, &g );
	f_inverse_square( 3.5, 0.5, &g );

	// add fields together and normalize
	f_sum( &f, &g, &f );
	f_normalize( &f );

	min.x = 0.5; min.y = 0.5;
	max.x = 9.5; max.y = 9.5;
	generate_grid( min, max, 10, 10, 0.01, &f );
*/

/*
	// Test 4 - outward radial grid with turbulence

	c.x = 2.5;
	c.y = 2.5; 
	f_concentric( c, &f );
	f_inverse_square( 1.5, 0.5, &f );
	//f_scale( 2.0, &f );
	f_turbulent( 0.75, 10, &g );
	f_sum( &f, &g, &f );
	f_normalize( &f );

	c_initialize( &k );
	c_set_run( 	10, 			// Number of subjects in run
				c, 				// Pick a vector, any vector
				0.25, 			// Initial step size
				1.0, 			// Step size proportional decrease per step
				false,			// Do not follow complementary lines
				&k );		

	generate_circle( 	c, 		// center of circle
						0.5, 	// radius of circle
						0.0,	// start angle
						5,		// number of runs around circle
						&k,
						&f );
*/

/*
	// Mt. Shuksan - 4 6x6 panels max.x =  24.0; max.y =  6.0;

	d.x = 1.0; d.y = 0.0;
	f_linear( d, &f );

	// four rotators in alternating directions
	for( i=0; i<4; i++) 
	{	
		c.x = 3.0 + 6.0 * i;	
		c.y = 3.0;
		// if(i%2) 
			f_spiral( c, -0.25,  1.0, &g );
		// else    f_spiral( c, -0.25, -1.0, &g );
		f_inverse_square( 1.5, 0.5, &g );
		//if( i%2 ) f_scale( -2.0, &g );
		//else 
		f_scale( 3.211, &g );
		f_sum( &f, &g, &f );
	}
	f_normalize( &f );

	start.x = 0.0;	start.y = 3.0;
	c_initialize( &k );
	c_set_run( 	100, 			// Number of subjects in run
			start, 				// Starting point
			0.5, 				// Initial step size
			1.0, 				// Step size proportional decrease per step
			false,				// Do not follow complementary lines
			&k );	
	c_set_bounds( f.min, f.max, &k );
	generate_cluster( &k, &f );	 
*/
	// randomly choose vector field

	// dipole, quadrupole, symmetric rotation centers

	// Radial stripes
	// all different kinds of stripes ... jaggy, squiggly, etc

	// favor symmetry and centering (but not always)

	// randomly choose # of starting points, # of items in run

	// potentially larger item in center

	// more types of objects - less frequent

	// free memory for buffers
	f_free( &f );
	f_free( &g );
}