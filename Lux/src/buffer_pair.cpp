#include "buffer_pair.hpp"

template< class T > buffer_pair< T >::buffer_pair() {
    image_pair.first = NULL;
    image_pair.second = NULL;
}

template< class T > buffer_pair< T >::buffer_pair( const std::string& filename ) {
    image_pair.first = std::make_unique< image< T > >( filename );
    image_pair.second = NULL;
}

template< class T > buffer_pair< T >::buffer_pair( const image< T > &img ) {
    image_pair.first = std::make_unique< image< T > >( img );
    image_pair.second = NULL;
}

template< class T > buffer_pair< T >::buffer_pair( vec2i dim ) {
    image_pair.first = std::make_unique< image< T > >( dim );
    image_pair.second = NULL;
}

/*
template< class T > buffer_pair< T >::buffer_pair( const buffer_pair< T >& bp ) {
    if ( bp.image_pair.first.get() != NULL ) image_pair.first = std::make_unique< image< T > >( *bp.get_image() );
    else image_pair.first = NULL;
    if( bp.image_pair.second.get() != NULL ) image_pair.second = std::make_unique< image< T > >( *bp.get_buffer() );
    else image_pair.second = NULL;
}
*/

template< class T > bool buffer_pair< T >::has_image() {
    return image_pair.first.get() != NULL;
}

template< class T > bool buffer_pair< T >::is_swapped() {
    return swapped;
}

template< class T > image< T >& buffer_pair< T >::get_image() {
    return *image_pair.first;
}

template< class T > const image< T >& buffer_pair< T >::get_image() const {
    return *image_pair.first;
}

template< class T > std::unique_ptr< image< T > >& buffer_pair< T >::get_image_ptr() { 
    return image_pair.first; 
}

template< class T > image< T >& buffer_pair< T >::get_buffer() {
    if( image_pair.second.get() == NULL ) image_pair.second.reset( new image< T >( *image_pair.first ) );
    return *image_pair.second;
}

template< class T > std::unique_ptr< image< T > >& buffer_pair< T >::get_buffer_ptr() { 
    if( image_pair.second.get() == NULL ) image_pair.second.reset( new image< T >( *image_pair.first ) );
    return image_pair.second; 
}

template< class T > void buffer_pair< T >::swap() { 
    image_pair.first.swap( image_pair.second ); 
    swapped = !swapped;
}

template< class T > void buffer_pair< T >::reset( const image< T >& img ) { 
    image_pair.first.reset( new image< T >( img ) );
    image_pair.second.reset( NULL );
    swapped = false;
}

template< class T > void buffer_pair< T >::reset( vec2i dim ) {
    // Code in here is problematic and has been causing segfaults
    if( image_pair.first.get() == NULL || image_pair.first->get_dim() != dim ) {
        std::cout << "buffer_pair::reset() " << dim.x << " " << dim.y << std::endl;
        image_pair.first.reset( new image< T >( dim ) );
        image_pair.second.reset( NULL );
        swapped = false;
    }
}

template< class T > void buffer_pair< T >::copy_first( const buffer_pair<T>& bp ) { 
    //std::cout << "buffer_pair::copy_first()" << std::endl;
    if( bp.image_pair.first.get()  != NULL ) {
        //std::cout << "buffer_pair::copy_first() - source pointer not NULL" << std::endl;
        if( image_pair.first.get() == NULL ) {
            //std::cout << "buffer_pair::copy_first() - dest pointer NULL" << std::endl;
            image_pair.first = std::make_unique< image< T > >( *bp.image_pair.first );
        }
        else {
            //std::cout << "buffer_pair::copy_first() - dest pointer not NULL" << std::endl;
            image_pair.first->copy( *bp.image_pair.first );
        }
    }
    else {
        //std::cout << "buffer_pair::copy_first() - source pointer NULL" << std::endl;
        image_pair.first.reset( NULL );
    }
}

template< class T > image< T >& buffer_pair< T >::operator () () {
    return get_buffer();
}

template class buffer_pair< frgb   >;   // fbuf
template class buffer_pair< ucolor >;   // ubuf
template class buffer_pair< vec2f  >;   // vbuf
template class buffer_pair< int    >;   // wbuf
template class buffer_pair< vec2i  >;   // obuf