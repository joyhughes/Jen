// Provides a random number generator and selected math functions to be used by linalg.h

#ifndef __JOY_RAND_HPP
#define __JOY_RAND_HPP

#include <random>

static std::random_device rd;    // non-deterministic generator
static std::mt19937 gen( rd() ); // start random engine
static std::uniform_real_distribution<float> rand1( 0.0f, 1.0f );
static std::uniform_int_distribution<unsigned int> fair_coin( 0, 1 ); 
static float rand_range( const float a, const float b ) { return a + ( b - a ) * rand1( gen ); }

// potentially unfair coin - returns 1 with probability a
static unsigned int weighted_bit( const float a ) { if( rand1( gen ) < a ) return 1; else return 0; }

// remainder function
static float remf( const float f ) { return f - floor( f ); }

// "true" modulo operator
static float tmodf( const float f, const float m ) { return remf( f / m ) * m; }

// Modulo operator that reflects in odd-numbered segments
static float rmodf( const float f, const float m ) { 
    float rat = f / m;
    int irat = floor( rat );
    if( irat % 2 ) return ( 1.0 - remf( rat ) ) * m;
    else return remf( rat ) * m;
}

#endif // __JOY_RAND_HPP