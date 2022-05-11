#ifndef __FRGB_HPP
#define __FRGB_HPP

#include <iostream>
#include <array>
#include <algorithm>

#define R 0
#define G 1
#define B 2

class frgb {
private:
    std::array< float, 3 > c;

public:
    // constructors
    frgb();
    frgb( float r, float g, float b );
    // TODO - add constructor from bracketed list

    // construct from 24 bit color
    frgb( unsigned char r, unsigned char g, unsigned char b );
    
    // might be more efficient to do conversion in image class as lookup table can be class member 
    // and conversion to a large block of frgb can use std::move
    // frgb( unsigned char r, unsigned char g, unsigned char b, std::unique_ptr<gamma_lut> t ); 
    // frgb( unsigned int in ) { bit shifty stuff}    
    // frgb( unsigned int in, std::unique_ptr<gamma_lut> t ) { bit shifty stuff }

    // destructor
    ~frgb();

    // iterators 
    std::array< float, 3>::iterator begin() { return c.begin(); }
    std::array< float, 3>::const_iterator begin() const { return c.begin(); }
    
    // accessors
    float& operator [] ( int i );
    const float& operator [] (int i) const { return c[ i ]; }

    inline float r();
    inline float g();
    inline float b();

    // returns single bytes per component - assumes [0.0, 1.0] range
    // clip or constrain out of range values before using
    inline unsigned char rc();
    inline unsigned char gc();
    inline unsigned char bc();
    

    // inline unsigned int ul() {} // bit shifty stuff

    // set component
    inline void setr( float r );
    inline void setg( float g );
    inline void setb( float b );
    inline void set( float r, float g, float b);
    // TODO - set from bracketed list

    inline void setrc( unsigned char r );
    inline void setgc( unsigned char g );
    inline void setbc( unsigned char b );
    inline void setc( unsigned char r, unsigned char g, unsigned char b );

    //inline void setul( unsigned int in );
    
    // arithmetic operators
    const frgb& operator += ( frgb rhs );
    const frgb& operator -= ( frgb rhs );
    const frgb& operator *= ( frgb rhs );

    const frgb& operator *= ( float rhs );
    const frgb& operator /= ( float rhs );

    frgb operator + ( frgb rhs );
    frgb operator - ( frgb rhs );
    frgb operator * ( frgb rhs );

    frgb operator * ( float rhs );
    frgb operator / ( float rhs);

    // I/O operators
    friend std::ostream &operator << ( std::ostream &out, const frgb& f ) { 
        out << "Linear - R: " << f.c[ R ] << " G: " << f.c[ G ] << " B: " << f.c[ B ] << "\n";
        return out;
    }

    void print_SRGB();

    void clamp( float minc, float maxc );   // clamp components to specified range
    void constrain();   // Clip to range [ 0.0, 1.0 ] but keep colors in proportion
};

#endif // __FRGB_HPP