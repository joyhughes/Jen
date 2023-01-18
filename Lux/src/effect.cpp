#include "effect.hpp"

// In this case t has no effect
template< class T > bool eff_vector_warp<T>::operator () ( buffer_pair< T > &buf, const float &t )  { 
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

// Fill image with color
template< class T > bool eff_fill<T>::operator () ( buffer_pair< T > &buf, const float &t )  { 
    if( buf.has_image() ) {
        if( bounded ) buf.get_image().fill( fill_color, bounds );
        else          buf.get_image().fill( fill_color );
        return true;
    }
    else {
        return false;
    }
}

// Noise effect
template< class T > bool eff_noise<T>::operator () ( buffer_pair< T > &buf, const float &t )  { 
    if( buf.has_image() ) {
        if( bounded ) buf.get_image().noise( a, bounds );
        else          buf.get_image().noise( a );
        return true;
    }
    else {
        return false;
    }
}

// Component effect - runs the same component effect n times
template< class T > bool eff_n<T>::operator () ( buffer_pair< T > &buf, const float &t )  { 
    if( buf.has_image() ) {
        for( int i = 0; i < n; i++ ) { if( !eff( buf ) ) return false; }
        return true;
    }
    else return false;
}

// Composite effect - runs a list of component effects
template< class T > bool effect<T>::operator () ( buffer_pair< T > &buf, const float &t )  {
    for( eff_fn fn : functions ) { if( !fn( buf, t ) ) return false; }
    return true;
}



