#ifndef __BUFFER_PAIR_HPP
#define __BUFFER_PAIR_HPP

#include "image.hpp"

// Used for double-buffered rendering. Owns pointer to image - makes a duplicate when 
// double-buffering is needed for an effect. Can be used in effect lists or for 
// persistent effects such as CA, melt, or hyperspace

template< class T > class buffer_pair {
    typedef std::unique_ptr< image< T > > image_ptr;
    std::pair< image_ptr, image_ptr > image_pair;
    bool swapped = false;
public:
    buffer_pair();
    buffer_pair( const std::string& filename );
    buffer_pair( const image< T >& img );       // copy image into buffer
    buffer_pair( vec2i dim );                  // create empty buffer of given dimensions
//  buffer_pair( const buffer_pair< T >& bp );  // copy constructor

    bool has_image();
    bool is_swapped();
    image< T >& get_image();
    const image< T >& get_image() const;
    std::unique_ptr< image< T > >& get_image_ptr();
    image< T >& get_buffer();                   // get buffer, create if necessary
    std::unique_ptr< image< T > >& get_buffer_ptr();
    void swap();                                // swap image and buffer

    /*
    void load( const std::string& filename ) { 
        image_pair.first.reset( new image< T >( filename ) );
        image_pair.second.reset( NULL );
    } */

    void reset( const image< T >& img );
    void reset( vec2i dim );

/*
    void set( const image< T >& img ) { 
        if( image_pair.first.get() == NULL ) image_pair.first->copy( img );
        else reset( img );
    }

    void set( const buffer_pair<T>& bp ) { 
        if( image_pair.first.get() == NULL ) image_pair.first->copy( bp.get_image() );
        // Copy second buffer if it exists
        //else if( bp.image_pair.second.get() != NULL ) reset( *bp.image_pair.second );
        else reset( bp.get_image() );
    }

    void set( vec2i& dim ) { 
        if( image_pair.first.get() == NULL ) image_pair.first->clear();
        else reset( dim );
    } */

    //copy first image
    void copy_first( const buffer_pair<T>& bp );

    image< T >& operator () ();
};

#endif // __BUFFER_PAIR_HPP