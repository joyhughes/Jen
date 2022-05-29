// Provides a random number generator and selected math functions to be used by linalg.h

#ifndef __JOY_RAND_HPP
#define __JOY_RAND_HPP

#include <random>

static std::random_device rd;   // non-deterministic generator
static std::mt19937 gen( rd() ); // start random engine
static std::uniform_real_distribution<float> rand1( 0.0f, 1.0f );
static float rand_range( const float a, const float b ) { return a + ( b - a ) * rand1(gen); }

// remainder function
static float remf( const float f ) { return f - floorf( f ); }

// "true" modulo operator
static float tmodf( const float f, const float m ) { return remf( f / m ) * m; }

// Modulo operator that reflects in odd-numbered segments
static float rmodf( const float f, const float m ) { 
    float rat = f / m;
    int irat = floorf( rat );
    if( irat % 2 ) return ( 1.0 - remf( rat ) ) * m;
    else return remf( rat ) * m;
}

#endif // __JOY_RAND_HPP