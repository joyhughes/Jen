#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "vect2.h"
#include "fimage.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

frgb fcolor( unsigned char r, unsigned char g, unsigned char b )
{
	frgb f;

	// here can also raise to the power of gamma
	f.r = r / 255.0;
	f.g = g / 255.0;
	f.b = b / 255.0;

	return f;
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

// *********************** FIMAGE FUNCTIONS *********************** 

void fimage_init( int xdim, int ydim, fimage *f )
{
	vect2 min = v_set( -1.0, -1.0 );
	vect2 max = v_set(  1.0,  1.0 );

	f->xdim = xdim;
	f->ydim = ydim;

	f->min = min;
	f->max = max;

	f->f = (frgb *)malloc( xdim * ydim * sizeof(frgb) );
}

void fimage_init_duplicate( fimage *in, fimage *out )
{
	fimage_init( in->xdim, in->ydim, out);
	fimage_set_bounds( in->min, in->max, out );
	fimage_copy_contents( in, out );
}

void fimage_free( fimage *f )
{
	free( f->f );
}

void fimage_set_bounds(  vect2 min, vect2 max, fimage *f )
{
	f->min = min;
	f->max = max;
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

// raise pixel values to power of two
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

void fimage_kaleidoscope( float k, float start_ang, bool reflect, fimage *in, fimage *out )
{
	int x,y;
	frgb *pin = in->f;
	frgb *pout = out->f;
	int xdim = out->xdim;
	int ydim = out->ydim;
	vect2 position;
	float kang = 360.0 / k;
	float ang, rot_ang;
	int segment;

	for( y = 0; y<ydim; y++ ) {
		for( x = 0; x<xdim; x++ ) {
			position.x = ( 1.0 * x ) / xdim * ( out->max.x - out->min.x ) + out->min.x;
			position.y = ( 1.0 * y ) / ydim * ( out->max.y - out->min.y ) + out->min.y;

			ang = vtoa( position ) + start_ang;
			segment = (int) ( ang / kang );
			rot_ang = - segment * kang;
			if( reflect && (segment % 2) ) rot_ang += kang / 2.0 - (ang - segment * kang );
			
			position = v_rotate( position, rot_ang );

			*pout = fimage_sample( position, false, in );

			pout++;
		}
	}

}
