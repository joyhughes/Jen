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
struct eff_chooser;
struct element;
struct cluster;
template< class T > struct CA;
template< class T > struct eff_fill;
template< class T > struct eff_noise;
template< class T > struct eff_checkerboard;
template< class T > struct eff_grayscale;
template< class T > struct eff_invert;
template< class T > struct eff_rotate_components;
template< class T > struct eff_rgb_to_hsv;
template< class T > struct eff_hsv_to_rgb;
template< class T > struct eff_rotate_hue;
template< class T > struct eff_bit_plane;

template< class T > struct eff_crop_circle;
template< class T > struct eff_mirror;
template< class T > struct eff_turn;
template< class T > struct eff_flip;
template< class T > struct eff_feedback;
template< class T > struct eff_vector_warp;

// Vector field effects
template< class T > struct eff_complement;
template< class T > struct eff_radial;
template< class T > struct eff_cartesian;
template< class T > struct eff_rotate_vectors;
template< class T > struct eff_scale_vectors;
template< class T > struct eff_normalize;
template< class T > struct eff_inverse;
template< class T > struct eff_inverse_square;
template< class T > struct eff_concentric;
template< class T > struct eff_rotational;
template< class T > struct eff_spiral;
template< class T > struct eff_fermat_spiral;
template< class T > struct eff_vortex;
template< class T > struct eff_turbulent;
template< class T > struct eff_kaleidoscope;
template< class T > struct eff_radial_tile;
template< class T > struct eff_theta_rotate;
template< class T > struct eff_theta_swirl;
template< class T > struct eff_theta_rings;
template< class T > struct eff_theta_waves;
template< class T > struct eff_theta_saw;
template< class T > struct eff_theta_compression_waves;
template< class T > struct eff_radial_multiply;
template< class T > struct eff_position_fill;

// warp field effects
template< class T > struct eff_fill_warp;

typedef std::variant <
    std::shared_ptr< eff_identity >, 
    std::shared_ptr< eff_n >,
    std::shared_ptr< eff_composite >,
    std::shared_ptr< eff_chooser >,
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

    std::shared_ptr< eff_checkerboard< frgb > >,
    std::shared_ptr< eff_checkerboard< ucolor > >,
    std::shared_ptr< eff_checkerboard< vec2f > >,
    std::shared_ptr< eff_checkerboard< int > >,
    std::shared_ptr< eff_checkerboard< vec2i > >,
    
    std::shared_ptr< eff_grayscale< frgb > >,
    std::shared_ptr< eff_grayscale< ucolor > >,

    std::shared_ptr< eff_invert< frgb > >,
    std::shared_ptr< eff_invert< ucolor > >,

    std::shared_ptr< eff_rotate_components< frgb > >,
    std::shared_ptr< eff_rotate_components< ucolor > >,

    std::shared_ptr< eff_rgb_to_hsv< frgb > >,
    std::shared_ptr< eff_rgb_to_hsv< ucolor > >,

    std::shared_ptr< eff_hsv_to_rgb< frgb > >,
    std::shared_ptr< eff_hsv_to_rgb< ucolor > >,

    std::shared_ptr< eff_rotate_hue< frgb > >,
    std::shared_ptr< eff_rotate_hue< ucolor > >,

    std::shared_ptr< eff_bit_plane< ucolor > >,

    std::shared_ptr< eff_crop_circle< frgb > >,
    std::shared_ptr< eff_crop_circle< ucolor > >,
    std::shared_ptr< eff_crop_circle< vec2f > >,
    std::shared_ptr< eff_crop_circle< int > >,
    std::shared_ptr< eff_crop_circle< vec2i > >,

    std::shared_ptr< eff_mirror< frgb > >,
    std::shared_ptr< eff_mirror< ucolor > >,
    std::shared_ptr< eff_mirror< vec2f > >,
    std::shared_ptr< eff_mirror< int > >,
    std::shared_ptr< eff_mirror< vec2i > >,

    std::shared_ptr< eff_turn< frgb > >,
    std::shared_ptr< eff_turn< ucolor > >,
    std::shared_ptr< eff_turn< vec2f > >,
    std::shared_ptr< eff_turn< int > >,
    std::shared_ptr< eff_turn< vec2i > >,

    std::shared_ptr< eff_flip< frgb > >,
    std::shared_ptr< eff_flip< ucolor > >,
    std::shared_ptr< eff_flip< vec2f > >,
    std::shared_ptr< eff_flip< int > >,
    std::shared_ptr< eff_flip< vec2i > >,
    
    std::shared_ptr< eff_feedback< frgb > >,
    std::shared_ptr< eff_feedback< ucolor > >,
    std::shared_ptr< eff_feedback< vec2f > >,
    std::shared_ptr< eff_feedback< int > >,
    std::shared_ptr< eff_feedback< vec2i > >,

    std::shared_ptr< eff_vector_warp< frgb > >,
    std::shared_ptr< eff_vector_warp< ucolor > >,
    std::shared_ptr< eff_vector_warp< vec2f > >,
    std::shared_ptr< eff_vector_warp< int > >,
    std::shared_ptr< eff_vector_warp< vec2i > >,

    // vector field effects
    std::shared_ptr< eff_complement< vec2f > >,
    std::shared_ptr< eff_radial< vec2f > >,
    std::shared_ptr< eff_cartesian< vec2f > >,
    std::shared_ptr< eff_rotate_vectors< vec2f > >,
    std::shared_ptr< eff_scale_vectors< vec2f > >,
    std::shared_ptr< eff_normalize< vec2f > >,
    std::shared_ptr< eff_inverse< vec2f > >,
    std::shared_ptr< eff_inverse_square< vec2f > >,
    std::shared_ptr< eff_concentric< vec2f > >,
    std::shared_ptr< eff_rotational< vec2f > >,
    std::shared_ptr< eff_spiral< vec2f > >,
    std::shared_ptr< eff_fermat_spiral< vec2f > >,
    std::shared_ptr< eff_vortex< vec2f > >,
    std::shared_ptr< eff_turbulent< vec2f > >,
    std::shared_ptr< eff_kaleidoscope< vec2f > >,
    std::shared_ptr< eff_radial_tile< vec2f > >,
    std::shared_ptr< eff_theta_rotate< vec2f > >,
    std::shared_ptr< eff_theta_swirl< vec2f > >,
    std::shared_ptr< eff_theta_rings< vec2f > >,
    std::shared_ptr< eff_theta_waves< vec2f > >,
    std::shared_ptr< eff_theta_saw< vec2f > >,
    std::shared_ptr< eff_theta_compression_waves< vec2f > >,
    std::shared_ptr< eff_radial_multiply< vec2f > >,
    std::shared_ptr< eff_position_fill< vec2f > >,

    // warp field effects
    std::shared_ptr< eff_fill_warp< int > >

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