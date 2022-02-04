typedef struct FRGB
{
	float r,g,b;

} frgb;

typedef struct Palette
{
	int ncolors;
	float *indices;
	frgb *colors;
	
} palette;

// *********************** FRGB FUNCTIONS *********************** 

frgb fcolor( unsigned char r, unsigned char g, unsigned char b );

frgb palette_index( float a, palette *p );

frgb rainbow( float a );

void aurora_palette( palette *p );