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

    vortex( float diameter_init = 0.33f, 
            float soften_init = 0.25f, 
            float intensity_init = 1.0f, 
            vec2f center_orig_init = { 0.0f, 0.0f },
            bool revolving_init = false,
            int velocity_init = 1,
            vec2f center_of_revolution_init = { 0.0f, 0.0f } ) :
        diameter( diameter_init ),
        soften( soften_init ),
        intensity( intensity_init ), 
        center_orig( center_orig_init ),
        revolving( revolving_init ), 
        velocity( velocity_init ), 
        center_of_revolution( center_of_revolution_init ) {}


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

    vortex_field(   int n_init = 10, 
                    bool revolving_init = true,
                    float scale_factor_init = 0.5f,
                    float min_diameter_init = 0.33f, float max_diameter_init = 0.33f,
                    float min_soften_init = 0.25f, float max_soften_init = 0.25f,
                    float min_intensity_init = 1.0f, float max_intensity_init = 1.0f,
                    rotation_direction intensity_direction_init = RANDOM,
                    int min_velocity_init = 1, int max_velocity_init = 1,
                    rotation_direction velocity_direction_init = RANDOM,
                    float min_orbital_radius_init = 0.0f, float max_orbital_radius_init = 0.5f )
        :   n( n_init ), revolving( revolving_init ),
            scale_factor( scale_factor_init ),
            min_diameter( min_diameter_init ), max_diameter( max_diameter_init ), 
            min_soften( min_soften_init ), max_soften( max_soften_init ),
            min_intensity( min_intensity_init ), max_intensity( max_intensity_init ), 
            intensity_direction( intensity_direction_init ),
             min_velocity( 1 ), max_velocity( 1 ), velocity_direction( RANDOM ),
            min_orbital_radius( min_orbital_radius_init ), max_orbital_radius( max_orbital_radius_init ), 
            generated( false )  {}  
};

#define vector_field image< vec2f >

// I/O functions using template specialization
//template<> void vector_field::load( const std::string& filename );

class vf_tools {
    vector_field& img;

public:
    vf_tools( vector_field& img ) : img( img ) {}

    //vector_field& get_image() { return img; }

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

    void kaleidoscope( const vec2f& center = { 0.0f, 0.0f }, 
                        float segments = 6.0,                // Number of segments in kaleidoscope
                        float offset_angle = 0.0f,           // Beginning of first segment in degrees
                        float spin_angle = 0.0f,
                        bool reflect = true,                 // Reflect alternate segments
                        const std::function<float(float)>& func);               

    void position_fill();

    // TODO:  implement vector field visualization
    void visualize( image< frgb >& img ) const {}
    void visualize( image< ucolor >& img ) const {}
};

#endif // __VECTOR_FIELD_HPP
