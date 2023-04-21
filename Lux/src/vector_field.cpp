#include "linalg.h"
#include "vect2.hpp"
#include "vector_field.hpp"

vec2f vortex::operator () ( const vec2f& v, const float& t ) {
    vec2f center = center_orig;
    if( revolving ) center = center_of_revolution + linalg::rot( velocity * t * TAU, center_orig - center_of_revolution );
    vec2f out = complement( v - center );
    out = inverse( out, diameter, soften ) * intensity;
    return out;
}

vec2f vortex_field::operator () ( const vec2f& v, const float& t ) {
    if( !generated ) generate();
    vec2f out = { 0.0f, 0.0f };
    for( auto& vort : vorts ) out += vort( v, t );
    return out;
}

void vortex_field::generate() {
    vortex vort;
    std::uniform_int_distribution<> velocity_distribution( min_velocity, max_velocity );

    generated = true;
    for( int i = 0; i < n; i++ ) {
        vort.diameter =  rand_range( min_diameter,  max_diameter );
        vort.soften =    rand_range( min_soften,    max_soften );
        vort.revolving = revolving;

        vort.center_of_revolution = bounds.box_of_random();
        if( revolving ) vort.center_orig = vort.center_of_revolution + 
            linalg::rot( rand_range( 0.0f, TAU ), { 0.0f, rand_range( min_orbital_radius, max_orbital_radius ) } );
        else vort.center_orig = vort.center_of_revolution;
        if( revolving ) {
            vort.velocity = velocity_distribution( gen );
            switch ( velocity_direction )
            {
            case COUNTERCLOCKWISE:
                vort.velocity = abs( vort.velocity );
                break;

            case CLOCKWISE:
                vort.velocity = -abs( vort.velocity );
                break;
            
            case RANDOM:
                if( fair_coin( gen ) ) vort.velocity *= -1;
                break;

            case LAVA_LAMP:
                if( vort.center_of_revolution.x < ( bounds.b1.x + bounds.b2.x ) / 2.0f ) vort.intensity *= -1;     
                break;
            }
        }

        vort.intensity = rand_range( min_intensity, max_intensity );
        switch ( intensity_direction )
        {
        case COUNTERCLOCKWISE:
            vort.intensity = abs( vort.intensity );
            break;

        case CLOCKWISE:
            vort.intensity = -abs( vort.intensity );
            break;
        
       case RANDOM:
            if( fair_coin( gen ) ) vort.intensity *= -1.0f;
            break;

        case LAVA_LAMP:
            if( vort.center_of_revolution.x < ( bounds.b1.x + bounds.b2.x ) / 2.0f ) vort.intensity *= -1.0f;     
            break;
        }

        vorts.push_back( vort );
    }
}

// Use Newton's method to move along flow line proportional to step value
// Angle in degrees
vec2f vector_field::advect( const vec2f& v, const float& step, const float& angle, const bool& smooth, const image_extend& extend ) const
{
    if( angle == 0.0f ) return v +                                    sample( v, smooth, extend )   * step;
    else                return v + linalg::rot( angle / 360.0f * TAU, sample( v, smooth, extend ) ) * step;
}

// This overload receives a matrix parameter - more efficient if invoked repeatedly with the same angle
vec2f vector_field::advect( const vec2f& v, const float& step, const mat2f& m, const bool& smooth, const image_extend& extend ) const
{
    return v + linalg::mul( m, sample( v, smooth, extend ) ) * step;
}

void vector_field::complement() { for( auto& v : base ) { v = ::complement( v ); } mip_it(); }
void vector_field::radial()     { for( auto& v : base ) { v = ::radial( v );     } mip_it(); }
void vector_field::cartesian()  { for( auto& v : base ) { v = ::cartesian( v );  } mip_it(); }

void vector_field::rotate_vectors( const float& ang ) {
    mat2f m = linalg::rotation_matrix_2D( ang / 360.0f * TAU );
    for( auto& v : base ) { v = linalg::mul( m, v ); }
    mip_it();
}

void vector_field::normalize() { std::transform( base.begin(), base.end(), base.begin(), [] ( const vec2f& v ) { return linalg::normalize( v );  } ); mip_it();}

void vector_field::inverse( float diameter, float soften ) {
    if( diameter == 0.0f ) { fill( { 0.0f, 0.0f } ); }
    else for( auto& v : base ) { v = ::inverse( v, diameter, soften ); }
    mip_it();
}

void vector_field::inverse_square( float diameter, float soften ) {
    if( diameter == 0.0f ) { fill( { 0.0f, 0.0f } ); }
    else for( auto& v : base ) { v = ::inverse_square( v, diameter, soften ); }
    mip_it();
}

void vector_field::concentric( const vec2f& center ) { 
    position_fill();
    for( auto& v : base ) { v = v - center; }
    mip_it(); 
}

void vector_field::rotation( const vec2f& center ) { 
    concentric();
    complement();
}

void vector_field::spiral( const vec2f& center, const float& cscale, const float& rscale )
{
    vector_field buffer( *this );

    concentric();
    *this *= cscale;
    buffer.rotation();
    buffer *= rscale;
    *this += buffer;
    mip_it();
}

void vector_field::vortex( const ::vortex& vort, const float& t ) {
    vec2f center= vort.center_orig;
    if( vort.revolving ) center = vort.center_of_revolution + linalg::rot( vort.velocity * t * TAU, vort.center_orig - vort.center_of_revolution );
    rotation( center );
    inverse( vort.diameter, vort.soften );
    *this *= vort.intensity;
    mip_it();
}

void vector_field::turbulent( vortex_field& ca, const float& t ) {
    //if( !(ca.generated) ) ca.generate();
    fill( { 0.0f, 0.0f } );
    vector_field buffer( *this );
    // cavort cavort cavort
    for( auto& vort : ca.vorts ) { buffer.vortex( vort ); *this += buffer; }
    mip_it();
}
 
void vector_field::position_fill() { 
    auto v = base.begin();
    for( int y = 0; y < dim.y; y++ ) {
        for( int x = 0; x < dim.x; x++ ) {
            *v = bounds.bb_map( vec2f( x, y ), ipbounds );
            v++;
        }
    }
    // rather than using mip_it() here, calculate directly up hierarchy using bounding box
}

void vector_field::write_jpg(const std::string &filename, int quality) {
    image< frgb > img( dim );
    visualize( img );
    img.write_jpg( filename, quality );
}

void vector_field::write_png(const std::string &filename ) {
    image< ucolor > img( dim );
    visualize( img );
    img.write_png( filename );
}

void vector_field::write_file(const std::string &filename, file_type type, int quality ) {
    switch( type ) {
        case FILE_JPG: write_jpg( filename, quality ); break;
        case FILE_PNG: write_png( filename ); break;
        case FILE_BINARY: write_binary( filename ); break;
        default: std::cout << "fimage::write_file: unknown file type " << type << std::endl;
    }
}

