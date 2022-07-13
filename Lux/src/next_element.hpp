#ifndef __NEXT_ELEMENT_HPP
#define __NEXT_ELEMENT_HPP

#include <functional>
#include "image.hpp"
#include "vector_field.hpp"
#include "scene.hpp"

template < class T > struct advect_element {
    const vector_field& vf;
    const float step, angle;
    bool proportional;  // Step size proportional to size
    bool smooth;
    void operator () ( element< T >& el, const float& t ) { 
        if( proportional ) el.position = vf.advect( el.position, step * el.scale, angle, smooth, SAMP_REPEAT ); 
        else               el.position = vf.advect( el.position, step,            angle, smooth, SAMP_REPEAT ); 
    }

    advect_element( const vector_field& vf_init, const float& step_init = 1.0f, const float& angle_init = 0.0f, bool proportional_init = true, bool smooth_init = false )
        : vf( vf_init ), step( step_init ), angle( angle_init ), proportional( proportional_init ), smooth( smooth_init ) {}
};

template < class T > struct scale_ratio {
    const float ratio;
    void operator () ( element< T >& el, const float& t ) { el.scale *= ratio; }
    scale_ratio( const float& ratio_init ) : ratio( ratio_init ) {}
};

template< class T > struct next_element {
    typedef std::function< void( element< T >&, const float& ) > element_fn;

    int max_index;
    std::optional< bb2f > bounds;
    std::vector< element_fn > functions;

    bool operator () ( element< T >& el, const float& t ) { 
        el.index++;
        if( el.index >= max_index ) return false;
        for( auto fn : functions ) fn( el, t );
        // bounds check
        if( bounds.has_value() ) if( !bounds->in_bounds( el.position ) ) return false;
        return true; 
    }

    void add_function( element_fn fn ) { functions.push_back( fn ); }
    next_element() : max_index( 100 ) {}
    next_element( const int& max_index_init, const std::optional< bb2f >  bounds_init = std::nullopt ) 
        : max_index( max_index_init ), bounds( bounds_init ) {}
};

#endif // __NEXT_ELEMENT_HPP