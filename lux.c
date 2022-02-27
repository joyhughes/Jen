#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "frgb.h"
#include "vect2.h"
#include "joy_io.h"
#include "func_node.h"
#include "vfield.h"
#include "fimage.h"
#include "scene.h"

bool critical_run;
bool critical_element;
bool debug_me;

int main( int argc, char const *argv[] )
{

    fimage base, buffer, overlay, splat, result, mask, stencil;
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
		load_err = scene_load( argv[ 1 ], &scn );					
		if( !load_err ) return 0;
	}
	else { 
      printf("Usage: ./lux <scene>.json [nframes]\n");
      return 0; 
   	}

   	int nframes = 1;
   	if( argc >= 3) nframes = atoi( argv[ 2 ] );

	// allocate output filename with room for code and extension
	output_filename = (char*)malloc( strlen( scn.output_basename ) + 12 );    

   	// load base image
   	// initialize base image
	// origin in lower left


	// load splat image (use default bounds)
   	// load_err = fimage_load( scn.splat_file, &splat );					if( !load_err ) return 0;

	// crop or apply a ramp function to splat
	// information outside of unit circle will not be displayed properly
	//fimage_circle_crop( &splat );
	fimage_circle_ramp( &splat );
	
	for( frame = 0; frame < nframes; frame++ ) {
		time = 1.0 * frame / nframes;
		scene_render( time, &scn );
		printf( "frame %d\n", frame );
		sprintf( output_filename, "%s_%04d.jpg", scn.output_basename, frame);
		fimage_write_jpg( output_filename, &result );
	}



	scene_free( &scn );


	return 0;
}
