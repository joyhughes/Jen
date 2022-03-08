
typedef struct FIMAGE
{
	int xdim, ydim;	
	vect2 min, max;		// Bounds of image in cartesian space
	frgb *f;			// Pointer to array of frgb pixels
} fimage;


// *********************** FIMAGE FUNCTIONS *********************** 

// *********************** Initialize, set, and free *********************** 

void fimage_init( int xdim, int ydim, fimage *f );
void fimage_init_duplicate( fimage *in, fimage *out );
void fimage_copy_contents( fimage *in, fimage *out);
void fimage_stub( fimage *f );
void fimage_free( fimage *f );
void fimage_set_bounds(  vect2 min, vect2 max, fimage *f );

// *********************** Conversion functions *********************** 

void fimage_continuous( int channels, unsigned char *img, fimage *f );
void fimage_quantize( unsigned char *img, fimage *f );

// *********************** I/O functions *********************** 

int fimage_load( const char *filename, fimage *fimg );
void fimage_write_jpg(const char *filename, fimage *fimg );
void fimage_write_ppm( char *filename, fimage *fimg );

// *********************** Modification functions *********************** 

void fimage_reflect_y( int mirror, bool top_to_bottom, fimage *fimg );
void fimage_sum( fimage *a, fimage *b, fimage *result );
void fimage_subtract( fimage *a, fimage *b, fimage *result );
void fimage_max( fimage *a, fimage *b );
void fimage_fill( frgb color, fimage *f );
void fimage_color_filter( frgb color, fimage *f );
void fimage_brightness( float b, fimage *f );
void fimage_square( fimage *f );	// raise pixel values to power of two
void fimage_normalize( fimage *f );
void fimage_rotate( float theta, 
					vect2 cor, 					// Center of rotation
					fimage *in, fimage *out );
void fimage_translate( int xoff, int yoff, fimage *in, fimage *out );
void fimage_circle_crop( fimage *f );	// Colors black everything outside of a centered circle
void fimage_circle_ramp( fimage *f );
void fimage_clip( float min, float max, fimage *in );

// *********************** Sampling functions *********************** 

frgb fimage_index( int x, int y, fimage *f );	// look up pixel by integer indices
frgb fimage_sample( vect2 v, bool smooth, fimage *fimg );	// sample image given coordinate in linear space

// *********************** Masking functions *********************** 

void fimage_make_mask( float thresh, fimage *in, fimage *out );
void fimage_apply_mask( fimage *base, fimage *mask, fimage *result );

// *********************** Rendering Functions *********************** 

void fimage_splat( 
	vect2 center, 			// coordinates of splat center
	float scale, 			// radius of splat
	float theta, 			// rotation in degrees
	frgb tint,				// change the color of splat
	fimage *f, 				// image to be splatted upon
	fimage *g 				// image of the splat
	);
void test_splat( fimage *r, fimage *f );
