
#ifndef __EFFECT_HPP
#define __EFFECT_HPP

#include "image.hpp"

// Component effect - wrapper for warp with vector field
template< class T > struct eff_vector_warp {
    vector_field& vf;
    float step; 
    bool smooth;
    bool relative;
    image_extend extend;

    // In this case t has no effect
    bool operator () ( buffer_pair< T >& buf, const float& t = 0.0f ) { 
        if( buf.has_image() ) {
            // Use buffer_pair operator () to return reference to first member of pair
            buf.get_buffer().warp( buf(), vf, step, smooth, relative, extend );
            buf.swap();
            return true;
        }
        else {
            return false;
        }
    }

    eff_vector_warp( vector_field& vf_init, float step_init = 1.0f, bool smooth_init = false, bool relative = true, image_extend extend_init = SAMP_SINGLE ) : 
        vf( vf_init ), step( step_init ), smooth( smooth_init ), relative( true ), extend( extend_init ) {}
};

// Component effect - runs the same component effect n times
template< class T > struct eff_n {
    typedef std::function< bool ( buffer_pair< T >&, const float& ) > eff_fn;

    int n;
    eff_fn& eff;

    bool operator () ( buffer_pair< T >& buf, const float& t = 0.0f ) { 
        if( buf.get_image() ) {
            for( int i = 0; i < n; i++ ) { if( !eff( buf ) ) return false; }
            return true;
        }
        else return false;
    }

    void set_eff( const eff_fn& eff_init ) { eff = eff_init; }

    eff_n( const int& n_init, const eff_fn& eff_init ) : 
        n( n_init ), eff( eff_init ) {}
};

// Composite effect. Runs a stack of component effects.
template< class T > struct effect {
    typedef std::function< bool ( buffer_pair< T >&, const float& ) > eff_fn;   // eff_fn good!

    std::vector< eff_fn > functions; // component functions for effect

    bool operator () ( buffer_pair< T >& buf, const float& t = 0.0f ) {
        for( eff_fn fn : functions ) { if( !fn( buf, t ) ) return false; }
        return true;
    }

    void add_effect( const eff_fn& eff ) { functions.push_back( eff ); }
};

#endif // __EFFECT_HPP