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

typedef struct FRGB
{
	double r,g,b;
} frgb;

typedef struct FIMAGE
{
	long xdim, ydim;
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

	img = (unsigned char *)malloc( 3 * f->xdim * f->ydim );
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
	for( y = 0; y<f->ydim; y++)	
	{
		for( x = 0; x<f->xdim; x++)
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
		for( y = 0; y<f->ydim; y++)	
		{
			for( x = 0; x<f->xdim; x++)
			{
				p->r /= max;
				p->g /= max;
				p->b /= max;

				p++;
			}
		}
	}
}

int main(int argc, char const *argv[])
{
    int xdim, ydim, channels;
    frgb black;
    frgb filters[4];
    fimage b, f, r;
    char *basename;
    char *filename;
    unsigned char *img;
    int i;

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
      printf("darklight lightfile darkfile\n");
      return 0; 
   	}
	   
	basename = remove_ext( (char *)argv[1], '.', '/' );           // scan input filename for "." and strip off extension, put that in basename
	filename = (char*)malloc( strlen(basename) + 12 );    // allocate output filename with room for code and extension
	  
	// initialize base image
	fimage_init( xdim, ydim, &b );

	// convert base image to frgb
	continuous( channels, img, &b );
	free( img );

	// load second image
	img = stbi_load( argv[2], &xdim, &ydim, &channels, 0 );
    if(img == NULL) 
    {
       printf("Error in loading second image\n");
       return 0;
    }

	// initialize sum image
	fimage_init( xdim, ydim, &f );
	continuous( channels, img, &f );
	free( img );

	fimage_init( xdim, ydim, &r );
	fimage_subtract( &b, &f, &r );
	fimage_subtract( &f, &r, &r );

	// normalize and render
	fimage_normalize( &r );

	// save result
	quantize( img, &r );
	sprintf( filename, "%s_dark.jpg", basename);
	stbi_write_jpg(filename, xdim, ydim, 3, img, 100);

	free( img );
	fimage_free( &b );
	fimage_free( &f );
	fimage_free( &r );
	return 0;
}
