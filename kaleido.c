#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "vect2.h"
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
	float k, start_ang;
	unsigned char *img;
	int frame, nframes;

	if( argc >= 2) 
	{
		fimage_load( (char *)argv[1], &in );
		fimage_init_duplicate( &in, &out );

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

	basename = remove_ext( (char *)argv[1], '.', '/' );		// scan input filename for "." and strip off extension, put that in basename
	filename = (char*)malloc( strlen(basename) + 12 );		// allocate output filename with room for code and extension

	for( frame = 0; frame < nframes; frame++ ) {
			k = 1.0 + (11.0 * frame) / nframes;
			start_ang = (360.0 * frame) / nframes;
			fimage_kaleidoscope( k, start_ang, true, &in, &out );
			sprintf( filename, "%s_%04d.jpg", basename, frame ); 
			fimage_write_jpg( filename, &out );
			printf( " frame %d\n",frame );
	}
}