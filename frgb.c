#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "frgb.h"

// *********************** Initialization function *********************** 

frgb fcolor( unsigned char r, unsigned char g, unsigned char b )
{
	frgb f;

	// here can also raise to the power of gamma
	f.r = r / 255.0;
	f.g = g / 255.0;
	f.b = b / 255.0;

	return f;
}

// *********************** frgb arithmatic functions *********************** 

frgb frgb_multiply( frgb c1, frgb c2 )
{
	c1.r *= c2.r;
	c1.g *= c2.g;
	c1.b *= c2.b;

	return c1;
}

frgb frgb_add( frgb c1, frgb c2 )
{
	c1.r += c2.r;
	c1.g += c2.g;
	c1.b += c2.b;

	return c1;
}


frgb frgb_subtract( frgb c1, frgb c2 )
{
	c1.r -= c2.r;
	c1.g -= c2.g;
	c1.b -= c2.b;

	return c1;
}

// *********************** Palette functions *********************** 

// linear interpolation of colors through a palette
// array should have colors at 0.0 and 1.0
// color indicies in increasing order, no two the same
frgb palette_index( float a, palette *p )
{
	frgb c;
	int i;
	float blend;

	a = a - floor(a);

	for( i=0; i < p->ncolors-1; i++ ) {
		if( ( a >= p->indices[i] ) && ( a < p->indices[i+1] ) ) {
			blend = ( a - p->indices[i] ) / ( p->indices[i+1] - p->indices[i] );
			c.r = ( 1.0 - blend ) * p->colors[i].r + blend * p->colors[i+1].r;
			c.g = ( 1.0 - blend ) * p->colors[i].g + blend * p->colors[i+1].g;
			c.b = ( 1.0 - blend ) * p->colors[i].b + blend * p->colors[i+1].b;
		}
	}
	return c;
}

// deprecated - use rainbow_palette instead
frgb rainbow( float a )
{
	frgb c;

	// wrap around
	a = a - floor(a);

	a *= 6.0;

	if(a < 1.0) 
	{
		// red to yellow
		c.r = 1.0;
		c.g = a;
		c.b = 0.0;
	}
	else if( a < 2.0 )
	{
		// yellow to green
		c.r = 2.0 - a;
		c.g = 1.0;
		c.b = 0.0;
	}
	else if( a < 3.0 )
	{
		// green to cyan
		c.r = 0.0;
		c.g = 1.0;
		c.b = a - 2.0;
	}
	else if( a < 4.0 )
	{
		// cyan to blue
		c.r = 0.0;
		c.g = 4.0 - a;
		c.b = 1.0;
	}
	else if( a < 5.0 )
	{
		// blue to magenta
		c.r = a - 4.0;
		c.g = 0.0;
		c.b = 1.0;
	}
	else
	{
		// magenta to red
		c.r = 1.0;
		c.g = 0.0;
		c.b = 6.0 - a;
	}

//	printf("rainbow a=%02f\n r=%f\n g=%f\n b=%f\n",a,c.r,c.g,c.b);
	return c;
}

void aurora_palette( palette *p )
{
	p->ncolors = 4;
	p->indices = (float *)malloc( p->ncolors * sizeof(float) );
	p->colors  =  (frgb *)malloc( p->ncolors * sizeof(frgb) );

	p->indices[0]  = 0.0;
	p->colors[0].r = 0.0;
	p->colors[0].g = 1.0;
	p->colors[0].b = 0.0;

	p->indices[1]  = 0.5;
	p->colors[1].r = 0.0;
	p->colors[1].g = 0.0;
	p->colors[1].b = 1.0;

	p->indices[2]  = 0.75;
	p->colors[2].r = 1.0;
	p->colors[2].g = 0.0;
	p->colors[2].b = 2.0;

	p->indices[3]  = 1.0;
	p->colors[3].r = 0.0;
	p->colors[3].g = 1.0;
	p->colors[3].b = 0.0;
}

