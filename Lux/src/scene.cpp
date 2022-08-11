#include "scene.hpp"
#include "next_element.hpp"

template< class T > void element< T >::render( image< T >& in, const float& t ) { 
        float r = rotation;
        if( orientation_lock ) r += orientation;
        // run effect
        in.splat( position, scale, r, tint, img ); 
}

template< class T > void element< T >::operator () ( buffer_pair< T >& buf, const float& t ) {
    render( buf.get_image(), t );
}

template class element< frgb >;
template class element< ucolor >;
template class element< vec2f >;

// Recursively generate and render elements
template< class T > void cluster< T >::render( image< T >& img, const float& t ) { 
    element< T > el = root_elem;
    element_context< T > context( el, *this, img, t );
    el.render( img, t );
    while( next_elem( context ) ) { el.render( img, t ); }
    //( ( fimage & )img ).write_jpg( "hk_cluster.jpg", 100 ); // debug - save frame after each cluster                   
}

// change root element parameters for branching cluster
template< class T > void cluster< T >::set_root( element< T >& el ) {
    root_elem.position = el.position;
    root_elem.scale = el.scale;
    root_elem.rotation = el.rotation;
    root_elem.orientation = el.orientation;
    root_elem.orientation_lock = el.orientation_lock;
    root_elem.derivative = el.derivative;
    root_elem.derivative_lock = el.derivative_lock;
    root_elem.index = 0;
}

// render into a buffer pair. Rendering modifies image directly - does not require buffer swap.
// in this case the cluster serves as an effect functor (the effect being rendering)
template< class T > void cluster< T >::operator () ( buffer_pair< T >& buf, const float& t ) {
    render( buf.get_image(), t );
}

template class cluster< frgb >;
template class cluster< ucolor >;
template class cluster< vec2f >;