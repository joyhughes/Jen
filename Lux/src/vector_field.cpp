#include "linalg.h"
#include "vect2.hpp"
#include "vector_field.hpp"
#include <cmath>

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
                vort.velocity = std::abs( vort.velocity );
                break;

            case CLOCKWISE:
                vort.velocity = -std::abs( vort.velocity );
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
            vort.intensity = std::abs( vort.intensity );
            break;

        case CLOCKWISE:
            vort.intensity = -std::abs( vort.intensity );
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
vec2f vf_tools::advect( const vec2f& v, const float& step, const float& angle, const bool& smooth, const image_extend& extend ) const
{
    if( angle == 0.0f ) return v +                                    img.sample( v, smooth, extend )   * step;
    else                return v + linalg::rot( angle / 360.0f * TAU, img.sample( v, smooth, extend ) ) * step;
}

// This overload receives a matrix parameter - more efficient if invoked repeatedly with the same angle
vec2f vf_tools::advect( const vec2f& v, const float& step, const mat2f& m, const bool& smooth, const image_extend& extend ) const
{
    return v + linalg::mul( m, img.sample( v, smooth, extend ) ) * step;
}

void vf_tools::complement() { for( auto& v : img.base ) { v = ::complement( v ); } //img.mip_it(); 
}
void vf_tools::radial()     { for( auto& v : img.base ) { v = ::radial( v );     } //img.mip_it(); 
}
void vf_tools::cartesian()  { for( auto& v : img.base ) { v = ::cartesian( v );  } //img.mip_it(); 
}

void vf_tools::rotate_vectors( const float& ang ) {
    mat2f m = linalg::rotation_matrix_2D( ang / 360.0f * TAU );
    for( auto& v : img.base ) { v = linalg::mul( m, v ); }
    //img.mip_it();
}

void vf_tools::normalize() { 
    std::transform( img.base.begin(), img.base.end(), img.base.begin(), [] ( const vec2f& v ) { return linalg::normalize( v );  } ); 
    //img.mip_it();
}

void vf_tools::inverse( float diameter, float soften ) {
    if( diameter == 0.0f ) { img.fill( { 0.0f, 0.0f } ); }
    else for( auto& v : img.base ) { v = ::inverse( v, diameter, soften ); }
    //img.mip_it();
}

void vf_tools::inverse_square( float diameter, float soften ) {
    if( diameter == 0.0f ) { img.fill( { 0.0f, 0.0f } ); }
    else for( auto& v : img.base ) { v = ::inverse_square( v, diameter, soften ); }
    //img.mip_it();
}

void vf_tools::concentric( const vec2f& center ) { 
    position_fill();
    for( auto& v : img.base ) { v = v - center; }
    //img.mip_it(); 
}

void vf_tools::rotation( const vec2f& center ) { 
    concentric();
    complement();
}

void vf_tools::spiral( const vec2f& center, const float& cscale, const float& rscale )
{
    vector_field buffer( img );
    vf_tools buffer_tools( buffer );

    concentric();
    img *= cscale;
    buffer_tools.rotation();
    buffer *= rscale;
    img += buffer;
    //img.mip_it();
}

void vf_tools::fermat_spiral(const float& c)
{
    for( auto& v : img.base )               //Taking every vector in polar form
    {
        v.x = c * sqrtf(powf(v.x/c,2)+1.0);
        v.y = v.y + 137.508;
    }
}

void vf_tools::vortex( const ::vortex& vort, const float& t ) {
    vec2f center= vort.center_orig;
    if( vort.revolving ) center = vort.center_of_revolution + linalg::rot( vort.velocity * t * TAU, vort.center_orig - vort.center_of_revolution );
    rotation( center );
    inverse( vort.diameter, vort.soften );
    img *= vort.intensity;
    //img.mip_it();
}

void vf_tools::turbulent( vortex_field& ca, const float& t ) {
    //if( !(ca.generated) ) ca.generate();
    img.fill( { 0.0f, 0.0f } );
    vector_field buffer( img );
    vf_tools buffer_tools( buffer );

    // cavort cavort cavort
    for( auto& vort : ca.vorts ) { buffer_tools.vortex( vort ); img += buffer; }
    //img.mip_it();
}
 
// Assumes radial coordinates
void vf_tools::kaleidoscope(    const float& segments,                // Number of segments in kaleidoscope
                                const float& start,            // Beginning of first segment in degrees
                                const float& spin,
                                const bool& reflect ) {               // Reflect alternate segments
    std::cout << "vf_tools::kaleidoscope: segments = " << segments << std::endl;
    if( segments != 0.0f ) {
        if( reflect ) {
            float segments_adj = segments * 2.0f; 
            for( auto& v : img.base ) { v.THETA = rmodf( v.THETA + spin, 360.0f / segments_adj ) + start; } 
        }
        else          { for( auto& v : img.base ) { v.THETA = tmodf( v.THETA + spin, 360.0f / segments ) + start; } }
    }
}

// Input in radial coordinates, output in cartesian
void vf_tools::radial_tile( const float& segments, 
                            const float& levels, 
                            const vec2f& offset,   // offset within tile, 
                            const float& spin,     // rotation within tile
                            const float& expand,   // expansion within tile
                            const vec2f& zoom,  // zoom within tile, 
                            bool reflect_x, 
                            bool reflect_y ) {
    for( auto& v : img.base ) {
        v = vec2f( ( omodf( ( v.THETA - spin ) * segments, 360.0f, reflect_x ) / 180.0f - 1.0f - offset.x ) * zoom.x,
                   ( omodf( ( v.R - expand ) * levels, 1.0f, reflect_y ) *   2.0f - 1.0f - offset.y ) * zoom.y );
    }
}

void vf_tools::radial_multiply( const float& segments, 
                                const float& levels, 
                                const float& spin, 
                                const float& expand, 
                                const bool&  reflect, 
                                const bool&  reflect_levels ) {
    float segments_adj = segments;
    if( reflect ) segments_adj *= 2.0f;
    for( auto& v : img.base ) {
        v.R =     omodf( ( v.R - expand ) * levels, 1.0f, reflect_levels ); 
        v.THETA = omodf( ( v.THETA + spin ) * segments_adj, 360.0f, reflect ); 
    }      
}

// rotation shortcut assuming radial coordinates
void vf_tools::theta_rotate( const float& angle ) {
    for( auto& v : img.base ) { v.THETA += angle; }
}

void vf_tools::theta_swirl( const float& amount ) {
    if( amount != 0.0f ) {
        for( auto& v : img.base ) { v.THETA += v.R * amount; }
    }
}

void vf_tools::theta_rings( const float& n, const float& swirl, const float& alternate ) {
    if( n != 0.0f ) {
        float ring_number;
        for( auto& v : img.base ) { 
            ring_number = floorf( v.R * n );
            if( fmodf( ring_number, 2.0f ) == 0.0f ) v.THETA = alternate - v.THETA;
            else v.THETA += alternate;
            v.THETA += swirl * ring_number / n;
        }
    }
}

void vf_tools::theta_waves( const float& freq, const float& amp, const float& phase, const bool& const_amp ) {
    float freq_adj = freq * TAU;
    float phase_adj = phase * TAU / 360.0f;

    if( const_amp ) {
        for( auto& v : img.base ) { 
            if( v.R != 0.0f) v.THETA += amp / v.R * sin( freq_adj * v.R - phase_adj ); 
        }
    }
    else {
        for( auto& v : img.base ) { v.THETA += amp * sin( freq_adj * v.R - phase_adj ); }
    }
}
 
void vf_tools::theta_saw( const float& freq, const float& amp, const float& phase, const bool& const_amp ) {
    float freq_adj = freq * 4.0f;
    float phase_adj = phase * 4.0f / 360.0f;

    if( const_amp ) {
        for( auto& v : img.base ) { 
            if( v.R != 0.0f) v.THETA += amp / v.R * rmodf( freq_adj * v.R - phase_adj, 1.0f ); 
        }
    }
    else {
        for( auto& v : img.base ) { v.THETA += amp * rmodf( freq_adj * v.R - phase_adj, 1.0f ); }
    }
}
 
void vf_tools::theta_compression_waves( const float& freq, const float& amp, const float& phase, const bool& const_amp ) {
    float freq_adj = freq * TAU / 360.0f;
    float phase_adj = phase * TAU / 360.0f;

    if( const_amp ) {
        for( auto& v : img.base ) {
            if( v.R != 0.0f) v.THETA += amp / v.R * sin( freq_adj * v.THETA - phase_adj ); 
        }
    }
    else {
        for( auto& v : img.base ) { v.THETA += amp * sin( freq_adj * v.THETA - phase_adj ); }
    }
}

void vf_tools::position_fill() { 
    auto v = img.base.begin();
    for( int y = 0; y < img.dim.y; y++ ) {
        for( int x = 0; x < img.dim.x; x++ ) {
            *v = img.bounds.bb_map( vec2f( x, y ), img.ipbounds );
            v++;
        }
    }
    // rather than using //mip_it() here, calculate directly up hierarchy using bounding box
}

/*
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
*/

template<> void vector_field::write_file(const std::string &filename, file_type type, int quality ) {
    switch( type ) {
        //case FILE_JPG: write_jpg( filename, quality ); break;
        //case FILE_PNG: write_png( filename ); break;
        case FILE_BINARY: write_binary( filename ); break;
        default: std::cout << "fimage::write_file: unknown file type " << type << std::endl;
    }
}
