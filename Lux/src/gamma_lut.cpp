// Look up table to convert between floating-point and integer RGB colors

#include "gamma_lut.hpp"
#include <cmath>
#include <iostream>

typedef union {
 
    float f;
    struct
    {
 
        // Order is important.
        // Here the members of the union data structure
        // use the same memory (32 bits).
        // The ordering is taken
        // from the LSB to the MSB.
        unsigned int mantissa : 23;
        unsigned int exponent : 8;
        unsigned int sign : 1;
 
    } raw;
    unsigned int ui;
} flubber;

// Deprecated – for debugging purposes only
void printBinary( int n, int i )
{
    // Prints the binary representation
    // of a number n up to i-bits.
    int k;
    for (k = i - 1; k >= 0; k--) {
 
        if ((n >> k) & 1)
            std :: cout << 1;
        else
            std :: cout << 0;
    }
}

// Function to convert real value
// to IEEE floating point representation
// Deprecated – for debugging purposes only
void printIEEE( flubber var )
{
    // Prints the IEEE 754 representation
    // of a float value (32 bits)
 
    std :: cout << var.raw.sign;
    printBinary(var.raw.exponent, 8);
    std :: cout << " | ";
    printBinary(var.raw.mantissa, 23);
    std :: cout << "\n";
}

gamma_LUT :: gamma_LUT( float gamma ) {
  this->gamma = gamma;

  for ( int i = 0; i < 256; ++i) {
    SRGB_to_linear_LUT[ i ] = powf( i / 255.0f, gamma );
  }

  flubber flub;
  for ( unsigned int i=0; i < lts_entries; i++ ) {
    flub.ui = 0x38000000 | ( i << lts_shift );                        // 0 | 01110000 | 00000000000000000000000  (black magic)
    linear_to_SRGB_LUT[i] = (unsigned char)( powf( flub.f, 1.0f / 2.2f ) * 255 );
  }
} 

float gamma_LUT :: SRGB_to_linear(unsigned char index) { return SRGB_to_linear_LUT[ index ]; }

// may be more efficient to do reinterpret_cast on array of pointers to float
unsigned char gamma_LUT :: linear_to_SRGB( float index ) {
  // Cases for table underflow (SRGB value less than two) and overflow
  if( index < 0.000032f ) {
    if( index <= 0.0f ) return 0x00;
    else return 1;
  }
  if( index >= 1.0 ) return 0xff;

  flubber flub;
  flub.f = index;
  return linear_to_SRGB_LUT[ ( flub.ui & 0x07ff8000 ) >> lts_shift ];  // 0 | 00001111 | 11111111000000000000000 (black magic)
}
