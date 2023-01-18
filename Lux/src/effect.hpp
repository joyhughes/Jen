
#ifndef __EFFECT_HPP
#define __EFFECT_HPP

#include "image.hpp"
#include "vector_field.hpp"

template< class T > struct eff_fill  {
    T& fill_color;
    bool bounded;
    bb2i bounds;

    void set_bounds( const bb2i& bounds_init ) { bounds = bounds_init; bounded = true; }

    bool operator () ( buffer_pair< T >& buf, const float& t = 0.0f );

    eff_fill( const T& fill_color_init ) : bounded( false ), fill_color( fill_color_init ) {}
    eff_fill( const T& fill_color_init, const bb2i& bounds_init ) : bounds( bounds_init ), fill_color( fill_color_init ) {}
};

template< class T > struct eff_noise  {
    float a;
    bool bounded;
    bb2i bounds;

    void set_bounds( const bb2i& bounds_init ) { bounds = bounds_init; bounded = true; }

    bool operator () ( buffer_pair< T >& buf, const float& t = 0.0f );

    eff_noise( float a_init = 1.0f ) : bounded( false ), a( a_init ) {}
    eff_noise( float a_init, const bb2i& bounds_init ) : bounded(true), bounds( bounds_init ), a( a_init ) {}
};

// Component effect - wrapper for warp with vector field
template< class T > struct eff_vector_warp {
    vector_field& vf;
    float step; 
    bool smooth;
    bool relative;
    image_extend extend;

    // In this case t has no effect
    bool operator () ( buffer_pair< T >& buf, const float& t = 0.0f );

    eff_vector_warp( vector_field& vf_init, float step_init = 1.0f, bool smooth_init = false, bool relative = true, image_extend extend_init = SAMP_SINGLE ) : 
        vf( vf_init ), step( step_init ), smooth( smooth_init ), relative( true ), extend( extend_init ) {}
};

// Component effect - runs the same component effect n times
// eff_n cool!
template< class T > struct eff_n {
    typedef std::function< bool ( buffer_pair< T >&, const float& ) > eff_fn;

    int n;
    eff_fn& eff;

    bool operator () ( buffer_pair< T >& buf, const float& t = 0.0f );

    void set_eff( const eff_fn& eff_init ) { eff = eff_init; }

    eff_n( const int& n_init, const eff_fn& eff_init ) : 
        n( n_init ), eff( eff_init ) {}
};

// Composite effect. Runs a stack of component effects.
template< class T > struct effect {
    typedef std::function< bool ( buffer_pair< T >&, const float& ) > eff_fn;   // eff_fn good!

    std::vector< eff_fn > functions; // component functions for effect

    bool operator () ( buffer_pair< T >& buf, const float& t = 0.0f );

    void add_effect( const eff_fn& eff ) { functions.push_back( eff ); }
};

#endif // __EFFECT_HPP