#ifndef __ANY_EFFECT_HPP
#define __ANY_EFFECT_HPP

#include <variant>
#include "frgb.hpp"
#include "ucolor.hpp"
#include "vect2.hpp"
#include "any_image.hpp"

struct eff_identity;
struct eff_n;
struct eff_composite;
struct element;
struct cluster;
template< class T > struct CA;
template< class T > struct eff_fill;
template< class T > struct eff_noise;
template< class T > struct eff_feedback;
template< class T > struct eff_vector_warp;

typedef std::variant <
    std::shared_ptr< eff_identity >, 
    std::shared_ptr< eff_n >,
    std::shared_ptr< eff_composite >,
    std::shared_ptr< element >,
    std::shared_ptr< cluster >,
//    std::shared_ptr< CA< frgb > >,
    std::shared_ptr< CA< ucolor > >,
//    std::shared_ptr< CA< vec2f > >,
//    std::shared_ptr< CA< int > >,
//    std::shared_ptr< CA< vec2i > >,

    std::shared_ptr< eff_fill< frgb > >,
    std::shared_ptr< eff_fill< ucolor > >,
    std::shared_ptr< eff_fill< vec2f > >,
    std::shared_ptr< eff_fill< int > >,
    std::shared_ptr< eff_fill< vec2i > >,

    std::shared_ptr< eff_noise< frgb > >,
    std::shared_ptr< eff_noise< ucolor > >,
    std::shared_ptr< eff_noise< vec2f > >,
    std::shared_ptr< eff_noise< int > >,
    std::shared_ptr< eff_noise< vec2i > >,

    std::shared_ptr< eff_feedback< frgb > >,
    std::shared_ptr< eff_feedback< ucolor > >,
    std::shared_ptr< eff_feedback< vec2f > >,
    std::shared_ptr< eff_feedback< int > >,
    std::shared_ptr< eff_feedback< vec2i > >,

    std::shared_ptr< eff_vector_warp< frgb > >,
    std::shared_ptr< eff_vector_warp< ucolor > >,
    std::shared_ptr< eff_vector_warp< vec2f > >,
    std::shared_ptr< eff_vector_warp< int > >,
    std::shared_ptr< eff_vector_warp< vec2i > >
> any_effect_fn_ptr;

struct element_context;

struct any_effect_fn {
    typedef std::function< void ( any_buffer_pair_ptr&, element_context& context) > effect_fn;
    any_effect_fn_ptr fn_ptr;
    effect_fn fn;
    std::string name;

    void operator () ( any_buffer_pair_ptr& buf, element_context& context ) { fn( buf, context ); }

    any_effect_fn();
    any_effect_fn( any_effect_fn_ptr fn_ptr, effect_fn fn, std::string name ) : fn_ptr( fn_ptr ), fn( fn ), name( name ) {}
};

#endif // __ANY_EFFECT_HPP