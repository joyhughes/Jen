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
	func_node position_node;	fnode_init_leaf( &position_node, 	fd_vect2( v_zero ) );

	func_node start_ang_node;	fnode_init_leaf( &start_ang_node, 	fd_float( start_ang ) );

	func_node time_node;		fnode_init_leaf( &time_node, 		fd_float( 0.0 ) );

	func_node m_max_node;		fnode_init_leaf( &m_max_node, 		fd_float( m_max ) );

	// build function tree
	func_node radial_node;			fnode_init_1( &radial_node, 		&fn_v_radial, 		&position_node );

	func_node radial_rot_node;		fnode_init_2( &radial_rot_node, 	&fn_v_add_y, 		&radial_node, 		&start_ang_node );

	func_node m_node;				fnode_init_2( &m_node, 				&fn_f_multiply, 	&m_max_node, 		&time_node );

	func_node radial_warp_node;		fnode_init_2( &radial_warp_node, 	&fn_v_multiply_y, 	&radial_rot_node, 	&m_node );

	func_node warp_node;			fnode_init_1( &warp_node, 			&fn_v_cartesian, 	&radial_warp_node );

	basename = remove_ext( (char *)argv[1], '.', '/' );		// scan input filename for "." and strip off extension, put that in basename
	filename = (char*)malloc( strlen(basename) + 12 );		// allocate output filename with room for code and extension

	for( frame = 0; frame < nframes; frame++ ) {

			time_node.leaf_val = fd_float( 1.0 * frame / nframes );
			fimage_warp( &warp_node, &position_node, &in, &out );

			sprintf( filename, "%s_%04d.jpg", basename, frame ); 
			fimage_write_jpg( filename, &out );
			printf( " frame %d\n",frame );
	}
}