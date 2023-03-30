#ifndef __VECTOR_FIELD_HPP
#define __VECTOR_FIELD_HPP

#include "image.hpp"

struct vortex {
    float diameter;   // float - Overall size of vortex
    float soften;     // float - Avoids a singularity in the center of vortex
    float intensity;  // float - Strength of vortex. How vortexy is it? Negative value swirls the opposite direction.
    vec2f center_orig; // vect2 - Initial position of vortex
    // other mathematical properties here ... vortex type?  ( inverse, inverse_square, donut, etc )

    // Animation parameters
    bool revolving;  // bool - does the vortex revolve around a center?
    int  velocity;   // float - Speed of revolution. Must be integer for animation to loop
    vec2f center_of_revolution;   // vect2 - vortex revolves around this point

    vortex( float diam = 0.33f, float soft = 0.25f, float inten = 1.0f, vec2f c_orig = { 0.0f, 0.0f } ) :
        diameter( diam ), soften( soft ), intensity( inten ), center_orig( c_orig ),
        revolving( false ), velocity( 0 ), center_of_revolution( { 0.0f, 0.0f } ) {}


    vec2f operator () ( const vec2f& v, const float& t = 0.0f );
};

struct vortex_field  {                  // parameters for field of vortices
    int n;                              // number of vortices in field    
    bb2f bounds;
    float scale_factor;                 // overall scaling factor

    float min_diameter, max_diameter;   // float - size of vortex
    float min_soften, max_soften;       // float - singularity avoidance
    float min_intensity, max_intensity; // float
    rotation_direction intensity_direction;  // object of RotationDirection
        // vortex type?  ( inverse, inverse_square, donut, etc )

    bool revolving;                     // do vortices revolve?
    int min_velocity, max_velocity;           // must be integer values for animation to loop
    rotation_direction velocity_direction;   // object of RotationDirection
    float min_orbital_radius, max_orbital_radius; // float

    std::vector< vortex > vorts;
    bool generated;                     // has this.generate() been run?
 
    void generate();

    vec2f operator () ( const vec2f& v, const float& t = 0.0f );

    vortex_field( int n_init = 10, bool revolving_init = true ) 
        : n( n_init ), scale_factor( 0.5f ),
        min_diameter( 0.33f ), max_diameter( 0.33f ), min_soften( 0.25f ), max_soften( 0.25f ),
        min_intensity( 1.0f ), max_intensity( 1.0f ), intensity_direction( RANDOM ),
        revolving( revolving_init ), min_velocity( 1 ), max_velocity( 1 ), velocity_direction( RANDOM ),
        min_orbital_radius( 0.0f ), max_orbital_radius( 0.5f ), generated( false )  {}  
};

class vector_field : public image< vec2f > {

public:
    vector_field() : image() {}     
    // creates image of particular size 
    vector_field( const vec2i& dims ) : image( dims ){}     
    vector_field( const vec2i& dims, const bb2f& bb ) : image( dims, bb ) {}     
    // copy constructor
    vector_field( const image< vec2f >& img ) : image( img ) {}     

    vec2f advect( const vec2f& v, const float& step, const float& angle = 0.0f, const bool& smooth = true, const image_extend& extend = SAMP_REPEAT ) const;
    vec2f advect( const vec2f& v, const float& step, const mat2f& m,            const bool& smooth = true, const image_extend& extend = SAMP_REPEAT ) const;

    void complement();
    void radial();
    void cartesian();

    void rotate_vectors( const float& ang );
    void normalize();
    void inverse( float diameter, float soften = 0.0f );
    void inverse_square( float diameter, float soften = 0.0f );

    void concentric( const vec2f& center = { 0.0f, 0.0f } );
    void rotation(   const vec2f& center = { 0.0f, 0.0f } );
    void spiral(     const vec2f& center = { 0.0f, 0.0f }, const float& cscale = 1.0f, const float& rscale = 1.0f );

    void vortex( const ::vortex& vort, const float& t = 0.0f );
    void turbulent( vortex_field& f,  const float& t = 0.0f );

    void position_fill();

    // TODO:  implement vector field visualization
    void visualize( image< frgb >& img ) const {}
    void visualize( image< ucolor >& img ) const {}

    void write_jpg( const std :: string& filename, int quality );
    void write_png( const std :: string& filename );
    void write_file( const std::string& filename, file_type type = FILE_JPG, int quality = 100 );

};

#endif // __VECTOR_FIELD_HPP
