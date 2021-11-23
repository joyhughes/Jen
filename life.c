#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

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

void text_render( unsigned int *out, int xdim, int ydim )
{
   int x,y;    // Indices for printing
   unsigned int *mm;

   mm = out;
   for( y=0; y<ydim; y++ )
   {
      printf("|");
      for( x=0; x<xdim; x++ )
      {      
         if(*mm) 
         {
            printf("*"); 
         }
         else 
         {
            printf(" "); 
         }
         mm++;
      }
      printf("|\n");
   }
   printf("\n");
}

// Take input file image and copy it properly into buffer
void unspool( unsigned char *img, unsigned int *buffer, int xdim, int ydim, int channels)
{
   int x,y;
   unsigned char *ip = img;
   unsigned int *mm = buffer;


   for( y=0; y<ydim; y++ )
   {
      for( x=0; x<xdim; x++ )
      {
         *mm = (unsigned long)*ip *     0x01000000    // alpha channel 
             + (unsigned long)*(ip+1) * 0x00010000    // red channel
             + (unsigned long)*(ip+2) * 0x00000100    // red channel
             + (unsigned long)*(ip+3) * 0x00000001;   // red channel
         mm++;
         ip += channels;
      }
   }
}

void render( unsigned int *out, int xdim, int ydim, char *filename )
{
   unsigned char *pixels = (unsigned char *) out;

   // TODO: optional fill alpha channel
   stbi_write_png(filename, xdim, ydim, 4, pixels, xdim * 4);
}

// One step of the cellular automaton
void iterate(unsigned int *in, unsigned int *out, int xdim, int ydim)
{
   int x,y,count;
   unsigned int *ul,*um,*ur,*ml,*mm,*mr,*dl,*dm,*dr;    //neighbor pointers
   unsigned int *result;
   

   // initialize pointers for the upper left corner - wrap around (toroidal topolgy)

   mm = in;
   ul = mm + xdim * ydim - 1;
   um = mm + (ydim - 1) * xdim;
   ur = mm + (ydim - 1) * xdim + 1;
   ml = mm + xdim - 1;
   mr = mm + 1;
   dl = mm + 2*xdim - 1;
   dm = mm + xdim;
   dr = mm + xdim + 1;

   result = out;

   for( y=0; y<ydim; y++ )
   {
      for( x=0; x<xdim; x++ )
      {
         // implement game of life rule 
         count = 0;
         if(*ul) count++; if(*um) count++; if(*ur) count++; if(*ml) count++; if(*mr) count++; if(*dl) count++; if(*dm) count++; if(*dr) count++;
         // count = *ul + *um + *ur + *ml + *mr + *dl + *dm + *dr;
         if( *mm )
         {
            // case where center cell is filled
            if(count == 2 | count==3) *result = 0xffffffff; else *result = 0;
         }
         else
         {
            // case where center cell is empty
            if(count==3) *result = 0xffffffff; else *result = 0;
         }   

         // increment pointers
         ul++; um++; ur++; ml++; mm++; mr++; dl++; dm++; dr++; result++;

         //deal with horizontal wrap 
         // pop left hand pointers to far left (previous row)
         if(x == 0) { ul -= xdim; ml -= xdim; dl -= xdim; }
         // move right hand pointers to far left
         if(x == xdim-2) { ur -= xdim; mr -= xdim; dr -= xdim; }
      }

      // move right and left colums down
      ul+=xdim; ml+=xdim; dl+=xdim; 
      ur+=xdim; mr+=xdim; dr+=xdim; 

      // deal with vertical wrap
      // pop upper pointers to upper row
      if(y == 0)      { ul -= xdim * ydim; um -= xdim * ydim; ur -= xdim * ydim; }

      // move lower pointers to upper row
      if(y == ydim-2) { dl -= xdim * ydim; dm -= xdim * ydim; dr -= xdim * ydim; }          
   }
}

// Command line: life <filename> <nsteps>
// TODO: File with all arguments and choice of rule w/ all params for rule

int main( int argc, char *argv[] ) 
{
   int x, y, xdim, ydim, channels;   
   int step, nsteps;
   unsigned int *a, *b;                // dynamically allocated image buffers for iterating
   unsigned int *in,*out,*placeholder; // pointers for buffer swapping
   unsigned char *img;                 // buffer for loading image 
   char *basename, *filename;

   /* unsigned int a[] = 
      {  0,0,0,0,0,
         0,0,0xffffffff,0,0,
         0,0,0xffffffff,0,0,
         0,0,0xffffffff,0,0,
         0,0,0,0,0 }; */

   if( argc >= 2) 
   {
      unsigned char *img = stbi_load( argv[1], &xdim, &ydim, &channels, 0 );
      if(img == NULL) 
      {
         printf("Error in loading the image\n");
         return 0;
      }
   }
   else
   { 
      printf("filename expected\n");
      return 0; 
   }
   
   basename = remove_ext( argv[1], '.', '/' );           // scan input filename for "." and strip off extension, put that in basename
   filename = (char*)malloc( strlen(basename) + 12 );     // allocate output filename with room for code and extension
   sprintf( filename, "%s%04d.png", basename, 0 );       // filenames will be <basename>0000.png, <basename>0000.png, etc
   printf( "filename: %s\n",filename);

   a = (unsigned int*)malloc( xdim * ydim * sizeof( unsigned int ) );    
   b = (unsigned int*)malloc( xdim * ydim * sizeof( unsigned int ) ); 


   if( channels==4 )
   {
      unspool(img, a, xdim, ydim, channels);
   }
   else 
   {
      // TODO: fill in pixels if channels != 4 + clear, fill in, or mess with alpha channel
      printf("channels = %d\n", channels);
      printf("Better generalize the unspooler\n");
      return 0;
   }
   

   nsteps = 10;
   if( argc >= 3) nsteps = atoi(argv[2]); // optionally specify number of steps (default 10)

   in = a; out = b;                       // initialize buffer pointers
   printf("render initial image\n");
   render(in, xdim, ydim, filename);

   for( step=0; step<nsteps; step++)
   {
      printf("step %d\n",step);
      sprintf( filename, "%s%04d.png", basename, step+1 );
      printf("iterate\n");
      iterate(in,out,xdim,ydim);
      printf("render\n");
      render( out, xdim, ydim, filename);
      placeholder = in; in = out; out = placeholder;  // swap buffers
   }

   return 0;
}
