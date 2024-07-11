
#ifndef __EFFECT_HPP
#define __EFFECT_HPP

#include "any_effect.hpp"
#include "buffer_pair.hpp"
#include "any_image.hpp"
#include "vector_field.hpp"
#include "warp_field.hpp"
#include "next_element.hpp"


typedef std::function< void ( any_buffer_pair_ptr& buf, element_context& context ) > effect_fn;
struct any_effect_fn;

// Identity effect - does nothing
struct eff_identity  {
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );
};

// Component effect - fills a buffer with a color
template< class T > struct eff_fill  {
    harness< T > fill_color;
    bool bounded;
    harness< bb2f > bounds;

    void set_bounds( const bb2i& bounds_init ) { bounds = bounds_init; bounded = true; }

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_fill() : bounded( false ) {
        T b;
        black( b );
        fill_color = b;
    }
    eff_fill( T& fill_color_init ) : bounded( false ), fill_color( fill_color_init ) {}
    eff_fill( T& fill_color_init, const bb2i& bounds_init ) : bounds( bounds_init ), fill_color( fill_color_init ) {}
};

typedef eff_fill< frgb > eff_fill_frgb;
typedef eff_fill< ucolor > eff_fill_ucolor;
typedef eff_fill< vec2f > eff_fill_vec2f;
typedef eff_fill< int > eff_fill_int;
typedef eff_fill< vec2i > eff_fill_vec2i;

template< class T > struct eff_noise  {
    harness< float > a;
    bool bounded;
    harness< bb2f > bounds;

    void set_bounds( const bb2i& bounds_init ) { bounds = bounds_init; bounded = true; }

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_noise( float a_init = 1.0f ) : bounded( false ), a( a_init ) {}
    eff_noise( float a_init, const bb2i& bounds_init ) : bounded(true), bounds( bounds_init ), a( a_init ) {}
};

typedef eff_noise< frgb > eff_noise_frgb;
typedef eff_noise< ucolor > eff_noise_ucolor;
typedef eff_noise< vec2f > eff_noise_vec2f;
typedef eff_noise< int > eff_noise_int;
typedef eff_noise< vec2i > eff_noise_vec2i;

template< class T > struct eff_grayscale {
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );
};

typedef eff_grayscale< frgb > eff_grayscale_frgb;
typedef eff_grayscale< ucolor > eff_grayscale_ucolor;

template< class T > struct eff_invert {
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );
};

typedef eff_invert< frgb > eff_invert_frgb;
typedef eff_invert< ucolor > eff_invert_ucolor;

template< class T > struct eff_rotate_colors {
    harness< int > r;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_rotate_colors( int r_init = 0 ) : r( r_init ) {}
};

typedef eff_rotate_colors< frgb > eff_rotate_colors_frgb;
typedef eff_rotate_colors< ucolor > eff_rotate_colors_ucolor;

template< class T > struct eff_crop_circle {
    harness< T > background;
    harness< float > ramp_width;
    
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );
};

typedef eff_crop_circle< frgb > eff_crop_circle_frgb;
typedef eff_crop_circle< ucolor > eff_crop_circle_ucolor;
typedef eff_crop_circle< vec2f > eff_crop_circle_vec2f;
typedef eff_crop_circle< int > eff_crop_circle_int;
typedef eff_crop_circle< vec2i > eff_crop_circle_vec2i;

template< class T > struct eff_mirror {
    bool reflect_x;
    bool reflect_y;
    bool top_to_bottom;    // reflect from top to bottom if true, otherwise bottom to top
    bool left_to_right;   // reflect from left to right if true, otherwise right to left

    harness< vec2f > center;

    image_extend extend;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_mirror( bool reflect_x_init = false, 
                bool reflect_y_init = true,
                bool top_to_bottom_init = true,
                bool left_to_right_init = true,
                vec2f center_init = vec2f( 0.0f, 0.0f ),
                image_extend extend_init = SAMP_SINGLE ) : 
        reflect_x( reflect_x_init ), 
        reflect_y( reflect_y_init ),
        top_to_bottom( top_to_bottom_init ),
        left_to_right( left_to_right_init ),
        center( center_init ),
        extend( extend_init )
        {}
};

typedef eff_mirror< frgb > eff_mirror_frgb;
typedef eff_mirror< ucolor > eff_mirror_ucolor;
typedef eff_mirror< vec2f > eff_mirror_vec2f;
typedef eff_mirror< int > eff_mirror_int;
typedef eff_mirror< vec2i > eff_mirror_vec2i;

template< class T > struct eff_turn {
    direction4 direction;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_turn( direction4 direction_init = D4_RIGHT ) : direction( direction_init ) {}
};

typedef eff_turn< frgb > eff_turn_frgb;
typedef eff_turn< ucolor > eff_turn_ucolor;
typedef eff_turn< vec2f > eff_turn_vec2f;
typedef eff_turn< int > eff_turn_int;
typedef eff_turn< vec2i > eff_turn_vec2i;

template< class T > struct eff_flip {
    bool flip_x, flip_y;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_flip( bool flip_x_init = true, bool flip_y_init = false ) : flip_x( flip_x_init ), flip_y( flip_y_init ) {}
};

typedef eff_flip< frgb > eff_flip_frgb;
typedef eff_flip< ucolor > eff_flip_ucolor;
typedef eff_flip< vec2f > eff_flip_vec2f;
typedef eff_flip< int > eff_flip_int;
typedef eff_flip< vec2i > eff_flip_vec2i;

// Component effect - wrapper for warp with vector field
template< class T > struct eff_vector_warp {
    harness< std::string > vf_name;    // used to reference vector field from queue
    harness< float > step; 
    harness< bool > smooth;
    harness< bool > relative;
    harness< image_extend > extend;

    // In this case t has no effect
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_vector_warp( const std::string& vf_name_init = "none", 
                     const float& step_init = 1.0f, 
                     const bool& smooth_init = false, 
                     const bool& relative = true, 
                     const image_extend& extend_init = SAMP_SINGLE ) : 
        vf_name( vf_name_init ), step( step_init ), smooth( smooth_init ), relative( true ), extend( extend_init ) {}
};

typedef eff_vector_warp< frgb > eff_vector_warp_frgb;
typedef eff_vector_warp< ucolor > eff_vector_warp_ucolor;
typedef eff_vector_warp< vec2f > eff_vector_warp_vec2f;
typedef eff_vector_warp< int > eff_vector_warp_int;
typedef eff_vector_warp< vec2i > eff_vector_warp_vec2i;

template< class T > struct eff_feedback {
    harness< std::string > wf_name; // used to reference warp field from queue

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_feedback( const std::string& wf_name_init = "none" ) : wf_name( wf_name_init ) {}
};

typedef eff_feedback< frgb > eff_feedback_frgb;
typedef eff_feedback< ucolor > eff_feedback_ucolor;
typedef eff_feedback< vec2f > eff_feedback_vec2f;
typedef eff_feedback< int > eff_feedback_int;
typedef eff_feedback< vec2i > eff_feedback_vec2i;


// Vector field effects

template< class T > struct eff_complement {
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_complement() {}
};

typedef eff_complement< vec2f > eff_complement_vec2f;

template< class T > struct eff_radial {
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_radial() {}
};

typedef eff_radial< vec2f > eff_radial_vec2f;

template< class T > struct eff_cartesian {
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_cartesian() {}
};

typedef eff_cartesian< vec2f > eff_cartesian_vec2f;

template< class T > struct eff_rotate_vectors {
    harness< float > angle;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_rotate_vectors( float angle_init = 0.0f ) : angle( angle_init ) {}
};

typedef eff_rotate_vectors< vec2f > eff_rotate_vectors_vec2f;

template< class T > struct eff_scale_vectors {
    harness< float > scale;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_scale_vectors( float scale_init = 1.0f ) : scale( scale_init ) {}
};

typedef eff_scale_vectors< vec2f > eff_scale_vectors_vec2f;

template< class T > struct eff_normalize {
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_normalize() {}
};

typedef eff_normalize< vec2f > eff_normalize_vec2f;

template< class T > struct eff_inverse {
    harness< float > diameter;
    harness< float > soften;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_inverse( float diameter_init = 1.0f, float soften_init = 0.0f ) : diameter( diameter_init ), soften( soften_init ) {}
};

typedef eff_inverse< vec2f > eff_inverse_vec2f;

template< class T > struct eff_inverse_square {
    harness< float > diameter;
    harness< float > soften;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_inverse_square( float diameter_init = 1.0f, float soften_init = 0.0f ) : diameter( diameter_init ), soften( soften_init ) {}
};

typedef eff_inverse_square< vec2f > eff_inverse_square_vec2f;

template< class T > struct eff_concentric {
    harness< vec2f > center;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_concentric( vec2f center_init = vec2f( 0.0f, 0.0f ) ) : center( center_init ) {}
};

typedef eff_concentric< vec2f > eff_concentric_vec2f;

template< class T > struct eff_rotational {
    harness< vec2f > center;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_rotational( vec2f center_init = vec2f( 0.0f, 0.0f ) ) : center( center_init ) {}
};

typedef eff_rotational< vec2f > eff_rotational_vec2f;

template< class T > struct eff_spiral {
    harness< vec2f > center;
    harness< float > angle;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_spiral( vec2f center_init = vec2f( 0.0f, 0.0f ), float angle_init = 0.0f ) : center( center_init ), angle( angle_init ) {}
};

typedef eff_spiral< vec2f > eff_spiral_vec2f;

template< class T > struct eff_vortex {
    harness< float > diameter;   // float - Overall size of vortex
    harness< float > soften;     // float - Avoids a singularity in the center of vortex
    harness< float > intensity;  // float - Strength of vortex. How vortexy is it? Negative value swirls the opposite direction.
    harness< vec2f > center_orig; // vect2 - Initial position of vortex
    // other mathematical properties here ... vortex type?  ( inverse, inverse_square, donut, etc )

    // Animation parameters
    bool revolving;  // bool - does the vortex revolve around a center?
    harness< int   > velocity;   // float - Speed of revolution. Must be integer for animation to loop
    harness< vec2f > center_of_revolution;   // vect2 - vortex revolves around this point

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_vortex( float diameter_init = 1.0f, 
                float soften_init = 0.0f, 
                float intensity_init = 1.0f, 
                vec2f center_orig_init = vec2f( 0.0f, 0.0f ), 
                bool revolving_init = false, 
                int velocity_init = 0, 
                vec2f center_of_revolution_init = vec2f( 0.0f, 0.0f ) ) : 
        diameter( diameter_init ), 
        soften( soften_init ), 
        intensity( intensity_init ), 
        center_orig( center_orig_init ), 
        revolving( revolving_init ), 
        velocity( velocity_init ), 
        center_of_revolution( center_of_revolution_init ) {}
};

typedef eff_vortex< vec2f > eff_vortex_vec2f;

template< class T > struct eff_turbulent {
    harness< int > n;                              // number of vortices in field    
    harness< bb2f > bounds;
    harness< float > scale_factor;                 // overall scaling factor

    harness< float > min_diameter, max_diameter;   // float - size of vortex
    harness< float > min_soften, max_soften;       // float - singularity avoidance
    harness< float > min_intensity, max_intensity; // float
    rotation_direction intensity_direction;  // object of RotationDirection
        // vortex type?  ( inverse, inverse_square, donut, etc )

    bool revolving;                     // do vortices revolve?
    harness< int > min_velocity, max_velocity;           // must be integer values for animation to loop
    rotation_direction velocity_direction;   // object of RotationDirection
    harness< float > min_orbital_radius, max_orbital_radius; // float

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_turbulent( int n_init = 10, bool revolving_init = true ) : 
        n( n_init ), 
        scale_factor( 0.5f ),
        min_diameter( 0.33f ), 
        max_diameter( 0.33f ), 
        min_soften( 0.25f ), 
        max_soften( 0.25f ),
        min_intensity( 1.0f ), 
        max_intensity( 1.0f ), 
        intensity_direction( RANDOM ),
        revolving( revolving_init ), 
        min_velocity( 1 ), 
        max_velocity( 1 ), 
        velocity_direction( RANDOM ),
        min_orbital_radius( 0.0f ), 
        max_orbital_radius( 0.5f )  {}
};

typedef eff_turbulent< vec2f > eff_turbulent_vec2f;

template< class T > struct eff_position_fill {
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );
};

typedef eff_position_fill< vec2f > eff_position_fill_vec2f;

template< class T > struct eff_kaleidoscope {
    harness< vec2f > center;
    harness< float > segments;
    harness< float > offset_angle;
    harness< float > spin_angle;
    harness< bool >  reflect;
    any_fn< float > swirl_fn;

    bool filled;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_kaleidoscope( vec2f center_init = vec2f( 0.0f, 0.0f ), float segments_init = 6.0f, float offset_angle_init = 0.0f, float spin_angle_init = 0.0f, bool reflect_init = true) : 
        center( center_init ), segments( segments_init ), offset_angle( offset_angle_init ), spin_angle( spin_angle_init ), reflect( reflect_init ), filled( false )  {}
};

typedef eff_kaleidoscope< vec2f > eff_kaleidoscope_vec2f;

// Warp field effects
template< class T > struct eff_fill_warp {
    harness< std::string > vf_name;
    harness< bool > relative;
    harness< image_extend > extend;

    bool filled;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_fill_warp( std::string vf_name_init = "none", 
                   bool relative_init = true, 
                   image_extend extend_init = SAMP_REPEAT ) : 
        vf_name( vf_name_init ), 
        relative( relative_init ), 
        extend( extend_init ) {}
};

typedef eff_fill_warp< int > eff_fill_warp_int;
    
    // Component effect - runs the same component effect n times
// eff_n cool!
struct eff_n {
    harness< int > n;
    any_effect_fn eff;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    void set_effect( any_effect_fn& eff_init );

//    eff_n( int n_init = 0 ) : n( n_init ) {}
//    eff_n( int& n_init, any_effect_fn& eff_init ) : n( n_init ), eff( eff_init ) {}
    eff_n( int n_init = 0 ) {}
    eff_n( int& n_init, any_effect_fn& eff_init ) {}
};

// Composite effect. Runs a stack of component effects.
struct eff_composite {
    std::vector< any_effect_fn > effects; // component functions for effect

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    void add_effect( const any_effect_fn& eff );
};

// Chooser effect. Runs one of a set of possible effects.
struct eff_chooser {
    std::vector< any_effect_fn > effects; // component functions for effect
    harness< int > choice;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    any_effect_fn& get_effect();
    void choose( int choice );
    void choose( const std::string& name );
    void add_effect( const any_effect_fn& eff );

    eff_chooser() : choice( 0 ) {}
};

#endif // __EFFECT_HPP
