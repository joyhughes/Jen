#ifndef __ANY_IMAGE_HPP
#define __ANY_IMAGE_HPP

#include "vect2.hpp"
#include "frgb.hpp"
#include "ucolor.hpp"
#include <variant>

class fimage;
class uimage;
class vector_field;
template< class T > class buffer_pair;

enum pixel_type
{
	PIXEL_FRGB,
	PIXEL_UCOLOR,
	PIXEL_VEC2F,
    PIXEL_NULL
};
typedef enum pixel_type pixel_type;

// generic image component                    
typedef std::variant<   std::monostate, // null option
                    std::reference_wrapper< frgb >, 
                    std::reference_wrapper< ucolor >, 
                    std::reference_wrapper< vec2f >
                > any_pixel;

// generic image
typedef std::variant<   std::monostate, // null option
                    std::reference_wrapper< image< frgb > >, 
                    std::reference_wrapper< image< ucolor > >, 
                    std::reference_wrapper< image< vec2f > >
                > any_image;

typedef std::variant<   std::monostate, // null option
                    std::reference_wrapper< buffer_pair< frgb > >, 
                    std::reference_wrapper< buffer_pair< ucolor > >, 
                    std::reference_wrapper< buffer_pair< vec2f > >
                > any_buffer_pair;

// std::visit calls resolve functor, returns optional wrapped reference if visiting type the same
template< class T > struct resolve {
    std::optional< T > operator () ( T& a   ) { return a; }
    std::optional< T > operator () ( auto a ) { return std::nullopt; }
};

// overload pattern for lambda functions
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

#endif // __ANY_IMAGE_HPP