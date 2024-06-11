#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "frgb.h"
#include "vect2.h"
#include "joy_io.h"

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

// next string in file
void file_get_string( FILE *fp, char *str, char *junk, bool *eof )
{
	int nargs;

	
	//nargs = fscanf(fp, "%[^\"]\"%[^\"]\"", junk, str);

	// must have junk - otherwise the string result goes into junk
	// future - fix no leading character case
	// Allowed characters - capital and lowercase, numbers, underscore and minus sign
	nargs = fscanf(fp, "%[^a-z^A-Z^0-9^_^-]%[a-zA-Z0-9_-]", junk, str);
	if( nargs != 2 ) *eof = true;
	else *eof = false;
	printf( "file_get_string: %s\n",str);
}

bool file_get_bool( FILE *fp, char *str, char *junk, bool *eof )
{
	int nargs;

	// must have junk - otherwise the string result goes into junk
	nargs = fscanf(fp, "%[^a-z^A-Z]%[a-zA-Z]", junk, str);
	if( nargs != 2 ) *eof = true;
	else *eof = false;
	printf( "file_get_bool: %s\n",str);
	return( (!strcmp( str, "true")) || (!strcmp( str, "True")) || (!strcmp( str, "TRUE")) );
}

int file_get_int( FILE *fp, char *str, char *junk, bool *eof )
{
	int result;
	int nargs;

	nargs = fscanf(fp, "%[^0-9^-]%[0-9-]", junk, str);
	if( nargs != 2 ) *eof = true;
	else *eof = false;
	result = atoi( str );
	printf( "file_get_int: %d\n",result);
	return( result );
}

float file_get_float( FILE *fp, char *str, char *junk, bool *eof )
{
	float result;
	int nargs;

	nargs = fscanf(fp, "%[^0-9^-]%[0-9+.Ee-]", junk, str);
	if( nargs != 2 ) {
		*eof = true;
		printf( "file_get_float: invalid number of arguments\n nargs = %d\n junk = '%s'\n str = '%s'\n", nargs, junk, str);
	}
	else *eof = false;
	result = ( float )atof( str );
	printf( "file_get_float: %f\n",result);
	return( result );
}

vect2 file_get_vect2( FILE *fp, char *str, char *junk, bool *eof )
{
	vect2 result;

	result.x = file_get_float( fp, str, junk, eof );
	if( *eof ) return result;

	result.y = file_get_float( fp, str, junk, eof );
	return result;
}

frgb file_get_frgb( FILE *fp, char *str, char *junk, bool *eof )
{
	frgb result;

	result.r = file_get_float( fp, str, junk, eof );
	if( *eof ) return result;

	result.g = file_get_float( fp, str, junk, eof );
	if( *eof ) return result;

	result.b = file_get_float( fp, str, junk, eof );
	return result;
}

char* bool_str( bool b, char *str )
{
	if( b ) sprintf( str, "true" );
	else	sprintf( str, "false" );
	return str;
}


