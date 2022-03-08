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
#include "fimage.h"

int main( int argc, char const *argv[] )
{
	fimage base, mask, result;
	char *filename, *basename;

	if( argc >= 4) 
	{
		// load image file
		fimage_load( argv[1], &base );
		fimage_load( argv[2], &mask );
		fimage_load( argv[3], &result );
	}
	else
	{ 
	  printf("usage ./mask base_img mask_img overlay_img k\n");
	  return 0; 
	}

	basename = remove_ext( (char *)argv[1], '.', '/' );		// scan input filename for "." and strip off extension, put that in basename
	filename = (char*)malloc( strlen(basename) + 24 );		// allocate output filename with room for code and extension
	sprintf( filename, "%s_mask.jpg", basename );

	fimage_apply_mask( &base, &mask, &result );

	fimage_write_jpg( filename, &result );

	return 1;
}