#ifndef __NEXT_ELEMENT_HPP
#define __NEXT_ELEMENT_HPP

#include <functional>
#include "image.hpp"
#include "vector_field.hpp"
#include "scene.hpp"

// component functor to move element along flow lines in a vector field using Newton's method
template < class T > struct advect_element {
    const vector_field& vf;
    const float step;
    bool proportional;  // Step size proportional to size
    bool smooth;
    bool operator () ( element_context< T >& context, const float& t );

    advect_element( const vector_field& vf_init, const float& step_init = 1.0f, const float& angle_init = 0.0f, bool proportional_init = true, bool smooth_init = false )
        : vf( vf_init ), step( step_init ), proportional( proportional_init ), smooth( smooth_init ) {}
};

// component functor to multiply scale by a constant factor
// how to generalize this? Change any property by any function?
template < class T > struct scale_ratio {
    const float ratio;
    bool operator () ( element_context< T >& context, const float& t ) { context.el.scale *= ratio; return true; }
    scale_ratio( const float& ratio_init ) : ratio( ratio_init ) {}
};

// A cluster_func to create single branches at regular intervals
// Can be specified for each level or can copy level above 
// ( Currently copies level above )

template < class T > struct angle_branch {

    int interval;           // number of elements between branches
    int offset;             // branching interval offset
    std::optional< int > mirror_offset; // optionally allow mirrored or alternating branches
    float size_prop;        // size proportional to parent element
    float min_scale;        // minimum element size for branching
    float branch_ang;       // branching angle in degrees

    void render_branch( const float& ang, element_context< T >& context, const float& t );
    bool operator () ( element_context< T >& context, const float& t = 0.0f );
    angle_branch( const int& interval_init, const int& offset_init = 0, const std::optional< int > mirror_offset_init = std::nullopt, const float& size_prop_init = 0.5f, const float& min_scale_init = 0.001f, const float& branch_ang_init = 60.0f ) :
        interval( interval_init ), offset( offset_init ), mirror_offset( mirror_offset_init ), size_prop( size_prop_init ), min_scale( min_scale_init ), branch_ang( branch_ang_init ) {}
};

/*
template < class T > struct angle_branch {

    cluster& branch;        
    int interval;           // number of elements between branches
    int offset;             // branching interval offset
    float size_prop;        // size proportional to parent element
    float min_scale;        // minimum element size for branching
    float branch_ang;       // branching angle in degrees
     

    bool operator () ( element_context< T >& context, const float& t = 0.0f );

    angle_branch( cluster& branch_init, const int& interval_init, const int& offset_init = 0, const float& size_prop_init = 0.5f, const float& min_scale_init = 0.01f, const float& branch_ang_init = 60.0f ) :
        branch( branch_init ), interval( interval_init ), offset( offset_init ), size_prop( size_prop_init ), min_scale( min_scale_init ), branch_ang( branch_ang_init ) {}
};
*/
// container functor to recursively generate elements in cluster
template< class T > struct next_element {
    typedef std::function< bool ( element_context< T >&, const float& ) > generative_fn;

    int max_index;                          // Maximum number of elements - prevents infinite loop
    std::optional< bb2f > bounds;           // Optional bounding box - iteration stops when element outside the box
    std::vector< generative_fn > functions;    // list of component functions

    bool operator () ( element_context< T >& context, const float& t );

    void add_function( generative_fn fn ) { functions.push_back( fn ); }
    next_element() : max_index( 100 ) {}
    next_element( const int& max_index_init, const std::optional< bb2f >  bounds_init = std::nullopt ) 
        : max_index( max_index_init ), bounds( bounds_init ) {}
};

#endif // __NEXT_ELEMENT_HPP