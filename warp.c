#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "frgb.h"
#include "vect2.h"
#include "func_node.h"
#include "fimage.h"


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


void fimage_warp( func_tree *ftree, func_node *warp_node, func_node *position_node, func_node* color_node, fimage *in, fimage *out )
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
			position_node->value = fd_vect2( position );

			ftree_reset( ftree );
			// printf( "fimage_warp x = %d, y = %d\n", x, y );
			warp = fnode_eval( warp_node ).v;
			// printf( "fimage_warp - fnode_eval complete\n" );

			*pout = fimage_sample( warp, false, in );
			//if( color_node != NULL ) *pout = frgb_multiply( *pout, fnode_eval( color_node ).c );

			pout++;
		}
	}
}

int main( int argc, char const *argv[] )
{
	fimage in, out;
	int xdim, ydim, channels;
	char *filename, *basename;
	unsigned char *img;
	int frame, nframes;
	func_tree warp_tree;
	

	if( argc >= 3) 
	{
		// load image file
		fimage_load( argv[1], &in );
		fimage_init_duplicate( &in, &out );
		// fimage_circle_crop( &in );

		// load function file
		ftree_load( &warp_tree, argv[ 2 ] );

	 	if(argc >= 4) nframes 		= atoi( (char *)argv[ 3 ] );
	 		else nframes = 1;
	}
	else
	{ 
	  printf("usage kaleido filename k\n");
	  return 0; 
	}

	// scan for parameters and results
	func_node *position_node = 	ftree_index( &warp_tree, "position" );
	func_node *time_node = 		ftree_index( &warp_tree, "time" );	
	func_node *result_node = 	ftree_index( &warp_tree, "result" );
	func_node *color_node = 	ftree_index( &warp_tree, "color_result" );

	basename = remove_ext( (char *)argv[1], '.', '/' );		// scan input filename for "." and strip off extension, put that in basename
	filename = (char*)malloc( strlen(basename) + 24 );		// allocate output filename with room for code and extension

	for( frame = 0; frame < nframes; frame++ ) {

			time_node->value = fd_float( 1.0 * frame / nframes );
			fimage_warp( &warp_tree, result_node, position_node, color_node, &in, &out );

			sprintf( filename, "./frames/%s_%04d.jpg", basename, frame ); 
			fimage_write_jpg( filename, &out );
			printf( " frame %d\n",frame );
	}
}