#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "vect2.h"
#include "fimage.h"
#include "func_node.h"

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

void fimage_warp( fnode *warp_node, fnode *position_node, fimage *in, fimage *out )
{
	int x,y;
	frgb *pout = out->f;
	int xdim = out->xdim;
	int ydim = out->ydim;
	vect2 position, rad, warp;

	for( y = 0; y<ydim; y++ ) {
		for( x = 0; x<xdim; x++ ) {
			position.x = ( 1.0 * x ) / xdim * ( out->max.x - out->min.x ) + out->min.x;
			position.y = ( 1.0 * y ) / ydim * ( out->max.y - out->min.y ) + out->min.y;
			position_node->leaf_val = position;

			warp = fnode_eval( warp_node );

			*pout = fimage_sample( warp, false, in );
			pout++;
		}
	}
}

int main( int argc, char const *argv[] )
{
	fimage in, out;
	int xdim, ydim, channels;
	char *filename, *basename;
	float k, m, start_ang;
	unsigned char *img;
	int frame, nframes;

	

	if( argc >= 2) 
	{
		// load image file
		fimage_load( (char *)argv[1], &in );
		fimage_init_duplicate( &in, &out );

		// todo: load function file

	 	if(argc >= 3) nframes 		= atoi( (char *)argv[2] );
	 		else nframes = 100;
	 	if(argc >= 4) start_ang 	= atof( (char *)argv[3] );
	 		else start_ang = 0.0;
	}
	else
	{ 
	  printf("usage kaleido filename k\n");
	  return 0; 
	}

	float m_max = 8.0;

	// initialize leaf nodes
	func_node position_node;	fnode_init_leaf( &position_node, v_zero );
	func_node start_ang_node;	fnode_init_leaf( &start_ang_node, v_set( start_ang, 0.0 ) );
	func_node time_node;		fnode_init_leaf( &time_node, v_zero );



	// build function tree
	func_node radial_node;		fnode_init( &r_node, &v_radial_2, &position_node, NULL );
	func_node m


	basename = remove_ext( (char *)argv[1], '.', '/' );		// scan input filename for "." and strip off extension, put that in basename
	filename = (char*)malloc( strlen(basename) + 12 );		// allocate output filename with room for code and extension

	for( frame = 0; frame < nframes; frame++ ) {
			/*k = 1.0 + (23.0 * frame) / nframes;
			start_ang = (360.0 * frame) / nframes;
			fimage_kaleidoscope( k, start_ang, true, &in, &out );*/

			time_node.leaf_val = v_set( 1.0 * frame / nframes, 0.0 );
			m = 2.0 * frame / nframes;
			float ang_factor = ( 1.0 / 60.0 ) * frame / nframes;
			printf( "kaleido main: ang_factor = %f \n", ang_factor);
			fimage_radial_offset( 0.0, ang_factor, true, &in, &out );

			sprintf( filename, "%s_%04d.jpg", basename, frame ); 
			fimage_write_jpg( filename, &out );
			printf( " frame %d\n",frame );

	}
}