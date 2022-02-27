#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "frgb.h"
#include "vect2.h"
#include "joy_io.h"

// next string in file
void file_get_string( FILE *fp, char *str, char *junk, bool *eof )
{
	int nargs;

	// must have junk - otherwise the string result goes into junk
	nargs = fscanf(fp, "%[^\"]\"%[^\"]\"", junk, str);
	if( nargs != 2 ) *eof = true;
	else *eof = false;
	printf( "file_get_string: %s\n",str);
}

bool file_get_bool( FILE *fp, char *str, char *junk, bool *eof )
{
	int nargs;

	// must have junk - otherwise the string result goes into junk
	nargs = fscanf(fp, "%[^a-z]%[a-z]", junk, str);
	if( nargs != 2 ) *eof = true;
	else *eof = false;
	printf( "file_get_bool: %s\n",str);
	return( !strcmp( str, "true") );
}

int file_get_int( FILE *fp, char *str, char *junk, bool *eof )
{
	int result;
	int nargs;

	nargs = fscanf(fp, "%[^0-9,^-]%[0-9,-]", junk, str);
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

	nargs = fscanf(fp, "%[^0-9,^-]%[0-9,-,+,.,E,e]", junk, str);
	if( nargs != 2 ) *eof = true;
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

	result.x = file_get_float( fp, str, junk, eof );
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

