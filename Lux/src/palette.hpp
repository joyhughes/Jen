#ifndef __PALETTE_HPP
#define __PALETTE_HPP

#include "frgb.hpp"
#include "ucolor.hpp"

typedef std::function< frgb ( const float& ) > palette_frgb;
typedef std::function< ucolor ( const unsigned char& ) > palette_ucolor;

struct palette_interpolate {
    std::vector< frgb > colors;
    std::vector< float > values;

    frgb operator () ( const float& value ) {
        if( value <= values[0] ) return colors[0];
        if( value >= values[values.size()-1] ) return colors[colors.size()-1];
        for( unsigned int i = 0; i < values.size()-1; i++ ) {
            if( value >= values[i] && value <= values[i+1] ) {
                float t = ( value - values[i] ) / ( values[i+1] - values[i] );
                return linalg::lerp( colors[i], colors[i+1], t );
            }
        }
        return frgb( { 0.0f, 0.0f, 0.0f } );
    }

    palette_interpolate( const std::vector< frgb >& colors, const std::vector< float >& values ) : colors( colors ), values( values ) {}
};

struct palette_LUT {
    std::vector< ucolor > colors;

    ucolor operator () ( const unsigned char& index ) { return colors[index]; }

    palette_LUT( const std::vector< ucolor >& colors ) : colors( colors ) {}
    palette_LUT( const palette_frgb &p ) {
        colors.resize( 256 );
        frgb c;
        for( unsigned char i = 0; i <= 255; i++ ) {
            c = p( (float)i / 255.0f );
            colors[i] = usetc( rc(c), gc(c), bc(c) );
        }
    }
};

#endif __PALETTE_HPP
