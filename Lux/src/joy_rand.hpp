// Provides a random number generator and selected math functions to be used by linalg.h and elsewhere

#ifndef __JOY_RAND_HPP
#define __JOY_RAND_HPP

#include <random>
#include "joy_concepts.hpp"

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

// Need to determine if interval is open or closed, and handle empty intervals
template< Scalar T > struct interval {
    T min, max;

    interval& operator = ( const interval& rhs ) {
        if ( this != &rhs ) {
            min = rhs.min;
            max = rhs.max;
            if ( min > max ) std::swap( min, max );
        }
        return *this;
    }

    interval( const T& min_init, const T& max_init ) : min( min_init ), max( max_init ) { 
        if ( min > max ) std::swap( min, max ); 
    }
};

typedef interval< float > interval_float;
typedef interval< int >   interval_int;

#endif // __JOY_RAND_HPP
