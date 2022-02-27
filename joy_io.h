void  file_get_string( FILE *fp, char *str, char *junk, bool *eof );

bool  file_get_bool(   FILE *fp, char *str, char *junk, bool *eof );

int   file_get_int(    FILE *fp, char *str, char *junk, bool *eof );

float file_get_float(  FILE *fp, char *str, char *junk, bool *eof );

vect2 file_get_vect2(  FILE *fp, char *str, char *junk, bool *eof );

frgb  file_get_frgb(   FILE *fp, char *str, char *junk, bool *eof ); 

char* bool_str( bool b, char *str );