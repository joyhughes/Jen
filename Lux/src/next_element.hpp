#ifndef __NEXT_ELEMENT_HPP
#define __NEXT_ELEMENT_HPP

#include <functional>
#include "image.hpp"
#include "vector_field.hpp"
#include "scene.hpp"

template< class T, class U > struct harness {
    typedef std::function< U ( const U&, const element_context< T >& ) > gen_fn;
    std::vector< gen_fn > functions;
    U val;

    void operator () ( const element_context< T >& context ) 
        { for( auto fn : functions ) val = fn( val , context ); }

    U operator * () { return val; }
    harness< T, U >& operator = ( const U& u ) { val = u; }

    void add_function( const gen_fn& fn ) { functions.push_back( fn ); }

    harness( const U& val_init = 0.0f ) : val( val_init ) {}
    harness( const harness& h ) {
        // std::cout << " harness called " << val << "\n";
        val = h.val;
        for (int i = 0; i < h.functions.size(); i++) 
            functions.push_back( h.functions[ i ] ); 
    }
};

// use concept here - need to be able to *= class U by float
template < class T, class U > struct ratio {
    harness< T, float > r;
    
    U operator () ( const U& u, const element_context< T >& context ) { r( context ); return *r * u; }

    ratio( const float& r_init ) : r( r_init ) {}
};

template < class T, class U > struct adder {
    harness< T, U > r;
    
    U operator () ( const U& u, const element_context< T >& context ) { r( context ); return *r + u; }

    adder( const float& r_init = 0.0f ) : r( r_init ) {}
};

template < class T > struct log_fn {
    float scale;
    float shift;

    float operator () ( const float& val, const element_context< T >& context ) 
        {   // std::cout << " log_fn: val = " << val << " scale = " << scale << " shift = " << shift << " el.scale = " << context.el.scale << "\n";
            return ( log10( val ) + shift ) * scale; }

    log_fn( const float& scale_init = 1.0f, const float& shift_init = 0.0f ) : scale( scale_init ), shift( shift_init ) {}
};

// generalized functor to change the scale of an element using a float_fn
template < class T > struct scale_fn {
    typedef std::function< float ( const float&, const element_context< T >& ) > float_fn;
    float_fn fn;

    bool operator () ( element_context< T >& context ) 
        { context.el.scale = fn( context.el.scale, context ); return true; }

    scale_fn( const float_fn& fn_init ) : fn( fn_init ) {}
};

// generalized functor to change the orientation of an element using a float_fn
template < class T > struct orientation_fn {
    typedef std::function< float ( const float&, const element_context< T >& ) > float_fn;
    float_fn fn;

    bool operator () ( element_context< T >& context ) 
        { context.el.orientation = fn( context.el.orientation, context ); return true; }

    orientation_fn( const float_fn& fn_init ) : fn( fn_init ) {}
};

// parameterizes the member function by float-converted index of element ( distance from beginning )
template < class T > struct index_param {
    typedef std::function< float ( const float&, const element_context< T >& ) > float_fn;
    float_fn fn;
    // future: add cushioney stuff like scale, offset, etc.

    float operator () ( const float& val, const element_context< T >& context ) {    
        return fn( context.el.index * 1.0f, context );
    }

    index_param( const float_fn& fn_init ) : fn( fn_init ) {}
};

// parameterizes the member function by scale of element ( distance from beginning )
template < class T > struct scale_param {
    typedef std::function< float ( const float&, const element_context< T >& ) > float_fn;
    float_fn fn;
    // future: add cushioney stuff like scale, offset, etc.

    float operator () ( const float& val, const element_context< T >& context ) {    
        return fn( context.el.scale, context );
    }

    scale_param( const float_fn& fn_init ) : fn( fn_init ) {}
};

// parameterizes the member function by scale of element ( distance from beginning )
template < class T > struct time_param {
    typedef std::function< float ( const float&, const element_context< T >& ) > float_fn;
    float_fn fn;
    // future: add cushioney stuff like scale, offset, etc.

    float operator () ( const float& val, const element_context< T >& context ) {    
        return fn( context.t, context );
    }

    time_param( const float_fn& fn_init ) : fn( fn_init ) {}
};

// Make orientation inversely proportional to size
template< class T > struct curly {
    harness< T, float > curliness;

    bool operator () ( element_context< T >& context ) {
        curliness( context );
        element< T >& el = context.el;
        if( el.scale > 0.0f ) el.orientation += *curliness / el.scale;
        return true;
    }
   
    curly( const float& curliness_init = 1.0f ): curliness( curliness_init ) {}
};

// Generalized oscillator
template< class T > struct wiggle {
    harness< T, float > wavelength;
    harness< T, float > amplitude;
    harness< T, float > phase;
    harness< T, float > wiggliness; // speed of wiggling

    float operator () ( const float& val, const element_context< T >& context  ) {
        wavelength( context ); amplitude( context ); phase( context ); wiggliness( context );
        if( *wavelength != 0.0f ) 
            return *amplitude * sin( ( val / *wavelength + *phase + *wiggliness * context.t ) * TAU );
        else return 0.0f; 
    }

    wiggle( const float& wavelength_init = 1.0f, const float& amplitude_init = 1.0f, const float& phase_init = 0.0f, const float& wiggliness_init = 0.0f ) 
        : wavelength( wavelength_init ), amplitude( amplitude_init ), phase( phase_init ), wiggliness( wiggliness_init ) {}
};

// composite of multiple sine waves
template< class T > struct squiggle { 
    harness< T, float > squish;
    harness< T, float > volume;
    harness< T, float > speed;
    harness< T, float > phase_shift;

    std::vector< wiggle< T > > wiggles;

    float operator () ( const float& val, const element_context< T >& context ) {
        squish( context ); volume( context ); speed( context ); phase_shift( context );
        float out = 0.0f;
        for( auto w : wiggles ) { out += volume * w( val * squish + phase_shift + speed * context.t, context ); }
        return out;
    }

    squiggle( const float& squish_init = 1.0f, const float& volume_init = 1.0f, const float& speed_init = 1.0f, const float& phase_shift_init = 1.0f )
        : squish( squish_init ), volume( volume_init ), speed( speed_init ), phase_shift( phase_shift_init ) {}
};

// component functor to move element along flow lines in a vector field using Newton's method
template< class T > struct advect_element {
    const vector_field& vf;
    const float step;
    bool proportional;  // Step size proportional to size
    bool smooth;

    bool operator () ( element_context< T >& context );

    advect_element( const vector_field& vf_init, const float& step_init = 1.0f, const float& angle_init = 0.0f, bool proportional_init = true, bool smooth_init = false )
        : vf( vf_init ), step( step_init ), proportional( proportional_init ), smooth( smooth_init ) {}
};

// A generative function to create single branches at regular intervals
// Can be specified for each level or can copy level above 
// ( Currently copies level above )
template< class T > struct angle_branch {
    int interval;           // number of elements between branches
    int offset;             // branching interval offset
    std::optional< int > mirror_offset; // optionally allow mirrored or alternating branches
    float size_prop;        // size proportional to parent element
    float branch_ang;       // branching angle in degrees
    float branch_dist;      // distance proportional to size of elements

    void render_branch( const float& ang, element_context< T >& context );
    bool operator () ( element_context< T >& context );
    angle_branch( const int& interval_init, const int& offset_init = 0, const std::optional< int > mirror_offset_init = std::nullopt, const float& size_prop_init = 0.7f, const float& branch_ang_init = 60.0f, const float& branch_dist_init = 0.7f ) :
        interval( interval_init ), offset( offset_init ), mirror_offset( mirror_offset_init ), size_prop( size_prop_init ), branch_ang( branch_ang_init ), branch_dist( branch_dist_init ) {}
};

template< class T > struct rotate_around {
    typedef std::function< void ( float&, element_context< T >& ) > float_fn;
    harness< T, vec2f > center;
    harness< T, float > ang_inc;    // angle increment in degrees
    float total_rotation;
    const std::unique_ptr< float_fn >& radial_function;

    bool operator () ( element_context< T >& context ) { 
        center( context ); ang_inc( context );
        // to do - replace with matrix operation
        if( context.el.index == 0 ) total_rotation = 0.0f;
        vec2f diff = context.el.position - center;
        diff = radial( diff );
        diff.THETA += ang_inc;
        total_rotation += ang_inc / 360.0f;
        if( radial_function.get() ) diff.R = *( radial_function.get() )( total_rotation, context );
        diff = cartesian( diff );
        context.el.position = center + diff;
    }

    rotate_around( vec2f center_init, float ang_inc_init, const std::unique_ptr< float_fn >& radial_function_init = NULL  ) 
        : center( center_init ), ang_inc( ang_inc_init ), radial_function( radial_function_init ), total_rotation( 0.0f ) {}
};

// container functor to recursively generate elements in cluster
template< class T > struct next_element {
    typedef std::function< bool ( element_context< T >& ) > generative_fn;

    int max_index;                          // Maximum number of elements - prevents infinite loop
    std::optional< bb2f > bounds;           // Optional bounding box - iteration stops when element outside the box
    std::vector< generative_fn > functions; // list of component functions

    bool operator () ( element_context< T >& context );

    void add_function( generative_fn fn ) { functions.push_back( fn ); }
    next_element() : max_index( 100 ) {}
    next_element( const int& max_index_init, const std::optional< bb2f >  bounds_init = std::nullopt ) 
        : max_index( max_index_init ), bounds( bounds_init ) {}
};

#endif // __NEXT_ELEMENT_HPP