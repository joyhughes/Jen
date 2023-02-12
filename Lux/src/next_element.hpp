#ifndef __NEXT_ELEMENT_HPP
#define __NEXT_ELEMENT_HPP

#include <functional>
#include "image.hpp"
#include "vector_field.hpp"

struct element;
struct cluster;
struct scene;
struct element_context;

template< class T > struct any_fn;
template<> struct any_fn< float >;
template<> struct any_fn< int >;
template<> struct any_fn< vec2f >;
template<> struct any_fn< vec2i >;
struct any_condition_fn;
struct any_gen_fn;

// change to template if possible:     typedef std::function< T ( const U&, const element_context& ) > gen_fn;
typedef std::function< bool  ( bool&, element_context&  ) > bool_fn; 
typedef std::function< float ( float&, element_context& ) > float_fn; 
typedef std::function< int   ( int&, element_context&   ) > int_fn; 
typedef std::function< vec2f ( vec2f&, element_context& ) > vec2f_fn; 
typedef std::function< vec2i ( vec2i&, element_context& ) > vec2i_fn; 

typedef std::function< bool ( element_context& ) > gen_fn;

template< class U > struct harness {
    std::vector< any_fn< U > > functions;
    U val;

    void operator () ( element_context& context );

    U  operator *  () { return  val; }
    U* operator -> () { return &val; }
    harness< U >& operator = ( const U& u ) { val = u; return *this; }

    void add_function( const any_fn< U >& fn );

    harness( const U& val_init = 0.0f ) : val( val_init ) {}
    harness( const harness& h );
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

template< class U > struct adder {
    harness< U > r;
    
    U operator () ( U& u, element_context& context ) { 
        r( context );
        //std::cout << "adder: " << *r << " + " << u << " = " << *r + u << std::endl; 
        return *r + u; 
    }

    adder( const U& r_init = 0 ) : r( r_init ) {}
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
template< class U > struct index_param {
    any_fn< U > fn;

    U operator () ( U& val, element_context& context );

    index_param();
    index_param( any_fn< U >& fn_init );
};

typedef index_param< float > index_param_float;
typedef index_param< int  >  index_param_int;
typedef index_param< vec2f > index_param_vec2f;
typedef index_param< vec2i > index_param_vec2i;

// parameterizes the member function by scale of element 
template< class U > struct scale_param {
    any_fn< U > fn;
    // future: add cushioney stuff like scale, offset, etc.

    U operator () ( U& val, element_context& context );

    scale_param();
    scale_param( any_fn< U >& fn_init );
};

typedef scale_param< float > scale_param_float;
typedef scale_param< int  >  scale_param_int;
typedef scale_param< vec2f > scale_param_vec2f;
typedef scale_param< vec2i > scale_param_vec2i;

// parameterizes the member function by time
template< class U > struct time_param {
    any_fn< U > fn;
    // future: add cushioney stuff like scale, offset, etc.

    U operator () ( U& val, element_context& context );

    time_param();
    time_param( any_fn< U >& fn_init );
};

typedef time_param< float > time_param_float;
typedef time_param< int  >  time_param_int;
typedef time_param< vec2f > time_param_vec2f;
typedef time_param< vec2i > time_param_vec2i;

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
        for( auto& w : wiggles ) { out += *volume * w( val * *squish + *phase_shift + *speed * context.t, context ); }
        return out;
    }

    squiggle( const float& squish_init = 1.0f, const float& volume_init = 1.0f, const float& speed_init = 1.0f, const float& phase_shift_init = 1.0f )
        : squish( squish_init ), volume( volume_init ), speed( speed_init ), phase_shift( phase_shift_init ) {}
}; */

// generalized functions - type gen_fn

// identity function - no effect
struct identity_gen_fn {
    bool operator () ( element_context& context ) { return true; }
};

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

/*
struct position_list {
    harness< std::vector< vec2f > > positions;

    bool operator () ( element_context& context );

    position_list( std::vector< vec2f > positions_init ) : positions( positions_init ) {}
};
*/

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

struct switch_condition {
    bool val;

    void on()  { val = true; }
    void off() { val = false; }
    void flip() { val = !val; }
    bool operator () ( bool& b, element_context& context ) { return val; }

    switch_condition( bool val_init = true ) : val( val_init ) {}
};

// Conditions for index_filter and depth_filter 
struct initial_element_condition   { bool operator () ( bool& b, element_context& context ); };
struct following_element_condition { bool operator () ( bool& b, element_context& context ); };
struct top_level_condition         { bool operator () ( bool& b, element_context& context ); };
struct lower_level_condition       { bool operator () ( bool& b, element_context& context ); };
// even, odd, etc.

struct random_condition {
    harness< float > p;

    bool operator () ( bool& b, element_context& context );

    random_condition( float p_init = 0.5f ) : p( p_init ) {}
};

struct random_sticky_condition {
    bool initialized;
    bool on;
    harness< float > p_start; //        probability that condition is fulfilled at initialization
    harness< float > p_change_true; //  probability of changing true conditon
    harness< float > p_change_false; // probability of changing false condition

    bool operator () ( bool& b, element_context& context );

    random_sticky_condition( float p_start_init = 0.5f, float p_change_true_init = 0.0f, float p_change_false_init = 0.0f ) :
        p_start( p_start_init ), p_change_true( p_change_true_init ), p_change_false( p_change_false_init ), initialized( false ), on( true ) {}
};

struct filter {
    bool c;
    std::vector< any_condition_fn > conditions;
    std::vector< any_gen_fn > functions;        // list of component functions, executed in order

    bool operator () ( element_context& context );
    void add_function(  const any_gen_fn& fn );
    void add_condition( const any_condition_fn& c );

    filter( bool c_init = true ) : c( c_init ) {}
};

// container functor to recursively generate elements in cluster
struct next_element {
    int max_index;                          // Maximum number of elements - prevents infinite loop
    std::optional< bb2f > bounds;           // Optional bounding box - iteration stops when element outside the box
    std::vector< any_gen_fn > functions;        // list of component functions, executed in order

    bool operator () ( element_context& context );
    void add_function( any_gen_fn fn );

    next_element() : max_index( 100 ) {}
    next_element( const int& max_index_init, const std::optional< bb2f >  bounds_init = std::nullopt ) 
        : max_index( max_index_init ), bounds( bounds_init ) {}
};

#endif // __NEXT_ELEMENT_HPP