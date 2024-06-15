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
    char *output_filename;
    time_t t;
    palette color_palette;
    scene scn;
    int frame;
    int load_err;			// Image load error code
    float time;

    critical_run = false;
    critical_element = false;
    debug_me = false;

	// start random engines
	// useful to keep things repeatable during testing
	//srand((unsigned) time(&t));
	srand( 13 );

    scene_init( &scn );

    // load scene file
	if( argc >= 2 ) {
		load_err = scene_load( &scn, argv[ 1 ] );					
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
	
	for( frame = 0; frame < nframes; frame++ ) {
		time = 1.0 * frame / nframes;
		scene_render( time, &scn );
		printf( "frame %d\n", frame );
		sprintf( output_filename, "%s_%04d.jpg", scn.output_basename, frame);
		fimage_write_jpg( output_filename, &( scn.result_fimg ) );
	}

	scene_free( &scn );


	return 0;
}
