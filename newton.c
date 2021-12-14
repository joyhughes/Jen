#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

// floating point rgb values can exceed 1 (or be negative "anticolors" for that matter)
// must be normalized and converted back to 24-bit color space to be meaningfully displayed

#define TAU 6.283185307179586

typedef struct FRGB
{
	double r,g,b;
} frgb;

typedef struct FIMAGE
{
	int xdim, ydim;
	frgb *f;			// Pointer to array of frgb pixels
} fimage;

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

void fimage_init( long xdim, long ydim, fimage *f )
{
	f->xdim = xdim;
	f->ydim = ydim;
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

	printf( "Quantize - xdim = %d, ydim = %d\n", f->xdim, f->ydim );

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

void fimage_crop( int xmin, int ymin, int xmax, int ymax, fimage *f, fimage *g )
{
	frgb *p;
	frgb *q;
	frgb *base;
	int x, y;

	p = g->f;
	base = f->f + ymin * f->xdim + xmin;
	for( y = 0; y < g->ydim; y++ )
	{
		q = base;
		for( x = 0; x < g->xdim; x++ )
		{
			*p = *q;  // fimage_index( x + xmin, y + ymin, f );
			p++;
			q++;
		}
		base += f->xdim;
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

int main(int argc, char const *argv[])
{
	int nfiles = 3;
    int xdim, ydim, channels;
    frgb black;
    fimage b, f, g, r;
    char *basename;
    char *filename;
    unsigned char *img;
    int i;
    int repeat = 1400;

    black.r = 0.0;	black.g = 0.0;	black.b = 0.0;

	// load base image
	if( argc >= 2 ) 
	{
      img = stbi_load( argv[1], &xdim, &ydim, &channels, 0 );
      if(img == NULL) 
      {
         printf("Error in loading the image\n");
         return 0;
      }
  	}
  	else
   	{ 
      printf("filename expected\n");
      return 0; 
   	}

   	if( argc >= 3 )
   	{
   		nfiles = atoi( argv[2] );
   	}
	   
	basename = remove_ext( (char *)argv[1], '.', '/' );   // scan input filename for "." and strip off extension, put that in basename
	filename = (char*)malloc( strlen(basename) + 12 );    // allocate output filename with room for code and extension
	  
	// initialize base image
	fimage_init( xdim, ydim, &f );
	fimage_init( xdim, ydim, &b );

	// convert base image to frgb
	continuous( channels, img, &b );
	free( img );

	// initialize sum image
	// fimage_init( xdim, ydim, &g );	
	fimage_init( xdim, ydim, &r );	//result
	fimage_fill( black, &r );	

	for( i = 1; i <= nfiles; i++ )
	{
		// create filename
      	sprintf( filename, "%s%04d.jpg", basename, ( i - 1 ) % repeat + 1 );
      	img = stbi_load( filename, &xdim, &ydim, &channels, 0 );

		// load in image, convert to frgb
		continuous( channels, img, &f );
		free( img );
		// subtract base image
		fimage_subtract( &f, &b, &f );
		//fimage_square( &f );

		// multiply color by filter - blend back with self (imperfect filter)
		fimage_filter( rainbow( 1.0 * ( i - 1 ) / nfiles ), &f );

		fimage_normalize( &f );

		//fimage_rotate( i * TAU / nfiles, &f, &g);
		//fimage_translate( ( i - nfiles / 2 ) * ( 4000 / nfiles ), 0, &f, &g);

		// add image to result
		fimage_sum( &f, &r, &r );
	}

	// optional - add base image back in
	fimage_sum( &r, &b, &r );

	// normalize and render
	fimage_normalize( &r );
	//fimage_circle_crop( &r );

	// save result
	quantize( img, &b );
	sprintf( filename, "%s_out.jpg", basename);
	stbi_write_jpg(filename, b.xdim, b.ydim, 3, img, 100);

	printf("free img\n");
	free( img );
	printf("free b\n");
	fimage_free( &b );
	printf("free f\n");
	fimage_free( &f );	
	printf("free g\n");
	// fimage_free( &g );
	printf("free r\n");
	fimage_free( &r ); 
	return 0; 
}
