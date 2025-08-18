#ifndef __ANY_IMAGE_HPP
#define __ANY_IMAGE_HPP

#include "buffer_pair.hpp"
#include <variant>

//template< class T > struct effect;
//template< class T > class buffer_pair;

enum pixel_type
{
	PIXEL_FRGB,
	PIXEL_UCOLOR,
	PIXEL_VEC2F,
	PIXEL_INT,
	PIXEL_VEC2I
};

typedef enum pixel_type pixel_type;

// generic image component                    
typedef std::variant< frgb, ucolor, vec2f, int, vec2i > any_pixel;

// generic image

typedef std::shared_ptr< image< frgb   > > fimage_ptr; 
typedef std::shared_ptr< image< ucolor > > uimage_ptr; 
typedef std::shared_ptr< image< vec2f  > > vfield_ptr; 
typedef std::shared_ptr< image< int    > > wfield_ptr;
typedef std::shared_ptr< image< vec2i  > > ofield_ptr;

typedef std::variant< fimage_ptr, uimage_ptr, vfield_ptr, wfield_ptr, ofield_ptr > any_image_ptr; 

static const fimage_ptr null_fimage_ptr = NULL;

// generic image

typedef std::shared_ptr< buffer_pair< frgb   > > fbuf_ptr; 
typedef std::shared_ptr< buffer_pair< ucolor > > ubuf_ptr; 
typedef std::shared_ptr< buffer_pair< vec2f  > > vbuf_ptr; 
typedef std::shared_ptr< buffer_pair< int    > > wbuf_ptr;
typedef std::shared_ptr< buffer_pair< vec2i  > > obuf_ptr;

typedef std::variant< fbuf_ptr, ubuf_ptr, vbuf_ptr, wbuf_ptr, obuf_ptr > any_buffer_pair_ptr; 

static ubuf_ptr null_buffer_pair_ptr = NULL;

// Helper function to get pixel type name from any_buffer_pair_ptr
inline std::string get_pixel_type_name(const any_buffer_pair_ptr& buf) {
    return std::visit([](auto&& buffer) -> std::string {
        using T = std::decay_t<decltype(buffer)>;
        if constexpr (std::is_same_v<T, fbuf_ptr>) return "frgb";
        else if constexpr (std::is_same_v<T, ubuf_ptr>) return "ucolor";
        else if constexpr (std::is_same_v<T, vbuf_ptr>) return "vec2f";
        else if constexpr (std::is_same_v<T, wbuf_ptr>) return "int";
        else if constexpr (std::is_same_v<T, obuf_ptr>) return "vec2i";
        else return "unknown";
    }, buf);
}

template< class T > void copy_buffer( T& to, const any_buffer_pair_ptr& from ) { 
	const T& f = std::get< T >( from );
	if( f.get() == NULL ) throw std::runtime_error( "copy_buffer(): attempt to copy null buffer" );
	to->copy_first( *f ); 
}
 
template< class T > image< T >& get_image( const any_buffer_pair_ptr& buf ) { 
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "get_image: no image in buffer" );
        return buf_ptr->get_image();
    }
	else throw std::runtime_error( "get_image: invalid buffer type" );
}

#endif // __ANY_IMAGE_HPP
