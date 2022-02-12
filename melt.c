#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "frgb.h"
#include "vect2.h"
#include "func_node.h"
#include "vfield.h"
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

int main( int argc, char const *argv[] )
{
	fimage in, out;
	vfield melt;
	int xdim, ydim, channels;
	char *filename, *basename;
	unsigned char *img;
	int frame, nframes;
	func_tree melt_tree;
	

	if( argc >= 3) 
	{
		// load image file
		fimage_load( argv[1], &in );
		fimage_init_duplicate( &in, &out );
		// fimage_circle_crop( &in );

		// load function file
		ftree_load( &melt_tree, argv[ 2 ] );

	 	if(argc >= 4) nframes 		= atoi( (char *)argv[ 3 ] );
	 		else nframes = 1;
	}
	else
	{ 
	  printf("usage kaleido filename k\n");
	  return 0; 
	}

	vfield_initialize( in.xdim, in.ydim, in.min, in.max, &melt );
	vfield_position_fill( &melt );
	// scan for parameters and results
	func_node *position_node = 	ftree_index( &melt_tree, "position" );
	//func_node *time_node = 		ftree_index( &warp_tree, "time" );	
	func_node *result_node = 	ftree_index( &melt_tree, "result" );

	basename = remove_ext( (char *)argv[1], '.', '/' );		// scan input filename for "." and strip off extension, put that in basename
	filename = (char*)malloc( strlen(basename) + 24 );		// allocate output filename with room for code and extension

	float step = 0.025;

	for( frame = 0; frame < nframes; frame++ ) {

			//time_node->leaf_val = fd_float( 1.0 * frame / nframes );
			fimage_melt( &melt, &in, &out );

			sprintf( filename, "./frames/%s_%04d.jpg", basename, frame ); 
			fimage_write_jpg( filename, &out );
			printf( " frame %d\n",frame );
			vfield_func( result_node, position_node, &melt );
	}
}