
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
    std::string vf_name;    // used to reference vector field from queue
    harness< float > step; 
    bool smooth;
    bool relative;
    image_extend extend;

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
    std::string wf_name; // used to reference warp field from queue

    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    eff_feedback( const std::string& wf_name_init = "none" ) : wf_name( wf_name_init ) {}
};

typedef eff_feedback< frgb > eff_feedback_frgb;
typedef eff_feedback< ucolor > eff_feedback_ucolor;
typedef eff_feedback< vec2f > eff_feedback_vec2f;
typedef eff_feedback< int > eff_feedback_int;
typedef eff_feedback< vec2i > eff_feedback_vec2i;

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

#endif // __EFFECT_HPP
