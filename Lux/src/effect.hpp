
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
