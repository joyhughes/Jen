#include "effect.hpp"

// In this case t has no effect
template< class T > void eff_vector_warp<T>::operator () ( buffer_pair< T > &buf, const float &t )  { 
    if( !buf.has_image() ) throw std::runtime_error( "eff_vector_warp: no image in buffer" );
    // Use buffer_pair operator () to return reference to first member of pair
    buf.get_buffer().warp( buf(), vf, step, smooth, relative, extend );
    buf.swap();
}

// Fill image with color
template< class T > void eff_fill<T>::operator () ( buffer_pair< T > &buf, const float &t )  { 
    if( !buf.has_image() ) throw std::runtime_error( "eff_fill: no image in buffer" );
    if( bounded ) buf.get_image().fill( fill_color, bounds );
    else          buf.get_image().fill( fill_color );
}

// Noise effect
template< class T > void eff_noise<T>::operator () ( buffer_pair< T > &buf, const float &t )  { 
    if( !buf.has_image() ) throw std::runtime_error( "eff_noise: no image in buffer" );
    if( bounded ) buf.get_image().noise( a, bounds );
    else          buf.get_image().noise( a );
}

template< class T > void feedback< T > :: operator () ( buffer_pair<T> &buf, const float &t )
{
    if( !buf.has_image() ) throw std::runtime_error( "feedback: no image in buffer" );
    buf.get_image().warp( buf(), wf, 1.0f, false, true, SAMP_SINGLE );
    buf.swap();
}

// Composite effect - runs a list of component effects
template< class T > void effect<T>::operator () ( buffer_pair< T > &buf, const float &t )  {
    for( eff_fn fn : functions ) { fn( buf, t )  }
}



