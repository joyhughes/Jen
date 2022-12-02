#ifndef __NEXT_ELEMENT_HPP
#define __NEXT_ELEMENT_HPP

#include <functional>
#include "image.hpp"
#include "vector_field.hpp"

struct element;
struct cluster;
struct scene;
struct element_context;

// change to template if possible:     typedef std::function< T ( const U&, const element_context& ) > gen_fn;
typedef std::function< bool  ( bool&, element_context&  ) > bool_fn; 
typedef std::function< float ( float&, element_context& ) > float_fn; 
typedef std::function< int   ( int&, element_context&   ) > int_fn; 
typedef std::function< vec2f ( vec2f&, element_context& ) > vec2f_fn; 
typedef std::function< vec2i ( vec2i&, element_context& ) > vec2i_fn; 

typedef std::function< bool ( element_context& ) > gen_fn;

template< class U > struct harness {
    typedef std::function< U ( U&, element_context& ) > harness_fn;
    std::vector< harness_fn > functions;
    U val;

    void operator () ( element_context& context ) 
        { for( auto fn : functions ) val = fn( val , context ); }

    U  operator *  () { return  val; }
    U* operator -> () { return &val; }
    harness< U >& operator = ( const U& u ) { val = u; }

    void add_function( const harness_fn& fn ) { functions.push_back( fn ); }

    harness( const U& val_init = 0.0f ) : val( val_init ) {}
    harness( const harness& h ) {
        val = h.val;
        for (int i = 0; i < h.functions.size(); i++) 
            functions.push_back( h.functions[ i ] ); 
    }
};

// harness functions

// Placeholder function that returns its first argument - used in default construction of parameter functions
template< class U > struct identity_fn { 
    U operator () ( U& in, element_context& context ) { return in; }
};

typedef identity_fn< int >   identity_int;
typedef identity_fn< float > identity_float;
typedef identity_fn< vec2i > identity_vec2i;
typedef identity_fn< vec2f > identity_vec2f;

static identity_int   identity_static_int;
static identity_float identity_static_float;
static identity_vec2f identity_static_vec2f;
static identity_vec2i identity_static_vec2i;

template< class U > struct adder {
    harness< U > r;
    
    U operator () ( U& u, element_context& context ) { 
        r( context ); 
        return *r + u; 
    }

    adder( const float& r_init = 0.0f ) : r( r_init ) {}
};

typedef adder< int >   adder_int;
typedef adder< float > adder_float;
typedef adder< vec2i > adder_vec2i;
typedef adder< vec2f > adder_vec2f;

struct log_fn {
    harness< float > scale;
    harness< float > shift;
    //float scale;
    //float shift;

    float operator () ( float& val, element_context& context ) 
        { return ( log10( val ) + *shift ) * *scale; }

    log_fn( const float& scale_init = 1.0f, const float& shift_init = 0.0f ) : scale( scale_init ), shift( shift_init ) {}
};

// use concept here - need to be able to *= class U by float
template< class U > struct ratio {
    harness< float > r;
    
    U operator () ( U& u, element_context& context ) { 
        r( context ); 
        return *r * u; 
    }

    ratio( const float& r_init = 1.0f ) : r( r_init ) {}
};

typedef ratio<float> ratio_float;
typedef ratio<vec2f> ratio_vec2f;

// Generalized oscillator
struct wiggle {
    harness< float > wavelength;
    harness< float > amplitude;
    harness< float > phase;
    harness< float > wiggliness; // speed of wiggling

    float operator () ( float& val, element_context& context  );

    wiggle( const float& wavelength_init = 1.0f, const float& amplitude_init = 1.0f, const float& phase_init = 0.0f, const float& wiggliness_init = 0.0f ) 
        : wavelength( wavelength_init ), amplitude( amplitude_init ), phase( phase_init ), wiggliness( wiggliness_init ) {}
};

// parameterizes the member function by float-converted index of element ( distance from beginning )
// potentially can be templated, or implemented as int_fn with a type converter function
struct index_param {
    float_fn fn;
    // future: add cushioney stuff like scale, offset, etc.

    float operator () ( float& val, element_context& context );

    index_param( const float_fn& fn_init = identity_static_float ) : fn( fn_init ) {}
};

// parameterizes the member function by scale of element 
struct scale_param {
    float_fn fn;
    // future: add cushioney stuff like scale, offset, etc.

    float operator () ( float& val, element_context& context );

    scale_param( const float_fn& fn_init = identity_static_float ) : fn( fn_init ) {}
};

// parameterizes the member function by time
struct time_param {
    float_fn fn;
    // future: add cushioney stuff like scale, offset, etc.

    float operator () ( float& val, element_context& context );

    time_param( const float_fn& fn_init = identity_static_float ) : fn( fn_init ) {}
};

// composite of multiple sine waves
/* struct squiggle { 
    harness< float > squish;
    harness< float > volume;
    harness< float > speed;
    harness< float > phase_shift;

    std::vector< wiggle > wiggles;

    float operator () ( float& val, element_context& context ) {
        squish( context ); volume( context ); speed( context ); phase_shift( context );
        float out = 0.0f;
        for( auto w : wiggles ) { out += *volume * w( val * *squish + *phase_shift + *speed * context.t, context ); }
        return out;
    }

    squiggle( const float& squish_init = 1.0f, const float& volume_init = 1.0f, const float& speed_init = 1.0f, const float& phase_shift_init = 1.0f )
        : squish( squish_init ), volume( volume_init ), speed( speed_init ), phase_shift( phase_shift_init ) {}
}; */

// generalized functions - type gen_fn

// generalized functor to change the orientation of an element using a float_fn
struct orientation_gen_fn {
    harness< float > orientation;

    bool operator () ( element_context& context );

    orientation_gen_fn( const float& orientation_init = 0.0f ) : orientation( orientation_init ) {}
};

// generalized functor to change the scale of an element using a float_fn
struct scale_gen_fn {
    harness< float > scale;

    bool operator () ( element_context& context );

    scale_gen_fn( const float& scale_init = 1.0f ) : scale( scale_init ) {}
};

// other element parameters - position, image / mask, etc

// component functor to move element along flow lines in a vector field using Newton's method
// (could use position_param + adder)
struct advect_element {
    // vector_field& vf;
    harness< vec2f > flow;
    harness< float > step;
    bool proportional;  // Step size proportional to size
    bool time_interval_proportional;
    bool orientation_sensitive;

    bool operator () ( element_context& context );

    advect_element( 
        const vec2f& flow_init = { 1.0f, 0.0f }, 
        const float& step_init = 1.0f, 
        const float& angle_init = 0.0f, 
        bool proportional_init = true, 
        bool time_interval_proportional_init = false, 
        bool orientation_sensitive_init = true ) : 

    flow( flow_init ), 
    step( step_init ), 
    proportional( proportional_init ), 
    time_interval_proportional( time_interval_proportional_init ),
    orientation_sensitive( orientation_sensitive_init ) {}
};

// A generative function to create single branches at regular intervals
// Can be specified for each level or can copy level above 
// ( Currently copies level above )
struct angle_branch {
    int interval;           // number of elements between branches
    int offset;             // branching interval offset
    std::optional< int > mirror_offset; // optionally allow mirrored or alternating branches
    harness< float > size_prop;        // size proportional to parent element
    harness< float > branch_ang;       // branching angle in degrees
    harness< float > branch_dist;      // distance proportional to size of elements
 
    void render_branch( const float& ang, element_context& context );
    bool operator () ( element_context& context );

    angle_branch(   const int& interval_init = 1,
                    const int& offset_init = 0, 
                    const std::optional< int > mirror_offset_init = std::nullopt, 
                    const float& size_prop_init = 0.7f, const float& branch_ang_init = 60.0f, 
                    const float& branch_dist_init = 0.7f ) :

        interval( interval_init ), offset( offset_init ), 
        mirror_offset( mirror_offset_init ), 
        size_prop( size_prop_init ), 
        branch_ang( branch_ang_init ), 
        branch_dist( branch_dist_init ) {}
};

// Make orientation inversely proportional to size
struct curly {
    harness< float > curliness;

    bool operator () ( element_context& context );
   
    curly( const float& curliness_init = 1.0f ): curliness( curliness_init ) {}
};

struct position_list {
    harness< std::vector< vec2f > > positions;

    bool operator () ( element_context& context );

    position_list( std::vector< vec2f > positions_init ) : positions( positions_init ) {}
};

/*struct rotate_around {
    harness< vec2f > center;
    harness< float > ang_inc;    // angle increment in degrees
    float total_rotation; 
    const std::unique_ptr< float_fn >& radial_function; // change this!

    bool operator () ( element_context& context ) { 
        center( context ); ang_inc( context );
        // to do - replace with matrix operation
        if( context.el.index == 0 ) total_rotation = 0.0f;
        vec2f diff = context.el.position - *center;
        diff = radial( diff );
        diff.THETA += *ang_inc;
        total_rotation += *ang_inc / 360.0f;
        if( radial_function.get() ) diff.R = *radial_function.get()( total_rotation, context );
        diff = cartesian( diff );
        context.el.position = *center + diff;
    }

    rotate_around( vec2f center_init, float ang_inc_init, const std::unique_ptr< float_fn >& radial_function_init = NULL  ) 
        : center( center_init ), ang_inc( ang_inc_init ), radial_function( radial_function_init ), total_rotation( 0.0f ) {}
};
*/

// Conditions for index_filter and depth_filter - included by default
bool initial_element(   bool& b, element_context& context );
bool following_element( bool& b, element_context& context );
bool top_level(         bool& b, element_context& context );
bool lower_level(       bool& b, element_context& context );
// even, odd, etc.

struct filter {
    bool c;
    std::vector< bool_fn > conditions;
    std::vector< gen_fn > functions;        // list of component functions, executed in order

    bool operator () ( element_context& context );
    void add_function(  gen_fn fn ) { functions.push_back( fn ); }
    void add_condition( bool_fn c ) { conditions.push_back( c ); }

    filter( bool c_init = true ) : c( c_init ) {}
};

// container functor to recursively generate elements in cluster
struct next_element {
    int max_index;                          // Maximum number of elements - prevents infinite loop
    std::optional< bb2f > bounds;           // Optional bounding box - iteration stops when element outside the box
    std::vector< gen_fn > functions;        // list of component functions, executed in order

    bool operator () ( element_context& context );
    void add_function( gen_fn fn ) { functions.push_back( fn ); }

    next_element() : max_index( 100 ) {}
    next_element( const int& max_index_init, const std::optional< bb2f >  bounds_init = std::nullopt ) 
        : max_index( max_index_init ), bounds( bounds_init ) {}
};

#endif // __NEXT_ELEMENT_HPP