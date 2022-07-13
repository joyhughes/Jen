#include "scene.hpp"
#include "next_element.hpp"

template< class T > void cluster< T >::render( image< T >& in, float t ) { 
    element< T > el = root_elem;
    el.render( in );
    while( next_elem( el, t ) ) {
        el.render( in );
    }
    // for( auto br : branches ) br->render( in, t );
}

template class cluster< frgb >;