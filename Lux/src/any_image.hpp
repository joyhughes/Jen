#ifndef __ANY_IMAGE_HPP
#define __ANY_IMAGE_HPP

#include "vect2.hpp"
#include "frgb.hpp"
#include "ucolor.hpp"
#include <variant>

class fimage;
class uimage;
class vector_field;
template< class T > struct effect;
template< class T > class buffer_pair;

enum pixel_type
{
	PIXEL_FRGB,
	PIXEL_UCOLOR,
	PIXEL_VEC2F
};
typedef enum pixel_type pixel_type;

// generic image component                    
typedef std::variant< frgb, ucolor, vec2f > any_pixel;

// generic image

typedef std::shared_ptr< image< frgb > >   fimage_ptr; 
typedef std::shared_ptr< image< ucolor > > uimage_ptr; 
typedef std::shared_ptr< image< vec2f > >  vfield_ptr; 

typedef std::variant< fimage_ptr, uimage_ptr, vfield_ptr > any_image_ptr; 

static const fimage_ptr null_fimage_ptr = NULL;

#endif // __ANY_IMAGE_HPP