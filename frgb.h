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

// *********************** Initialization function *********************** 

frgb fcolor( unsigned char r, unsigned char g, unsigned char b );

// *********************** frgb arithmatic functions *********************** 

frgb frgb_multiply( frgb c1, frgb c2 );
frgb frgb_add( frgb c1, frgb c2 );
frgb frgb_subtract( frgb c1, frgb c2 );

// *********************** Palette functions *********************** 

frgb palette_index( float a, palette *p );
frgb rainbow( float a );
void aurora_palette( palette *p );

