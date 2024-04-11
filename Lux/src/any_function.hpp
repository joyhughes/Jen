#ifndef __ANY_FUNCTION_HPP
#define __ANY_FUNCTION_HPP

#include <variant>
#include "next_element.hpp"
#include "UI.hpp"
#include "life_hacks.hpp"

template < class T > struct any_fn {};

typedef std::variant < 
    // harness functions
    std::shared_ptr< identity_float >,
    std::shared_ptr< adder_float >,
    std::shared_ptr< log_fn >,
    std::shared_ptr< time_fn >,
    std::shared_ptr< ratio_float >,
    std::shared_ptr< wiggle >,

    // parameter functions
    std::shared_ptr< index_param< float > >,
    std::shared_ptr< scale_param< float > >,
    std::shared_ptr< time_param<  float > >,

    // ui functions
    std::shared_ptr< slider_float >,
    std::shared_ptr< range_slider_float >

> any_float_fn_ptr;

template<> struct any_fn< float > {
    any_float_fn_ptr any_float_fn;
    float_fn fn;
    std::string name;

    float operator () ( float& val, element_context& context ) { return fn( val, context ); }

    any_fn< float>() : name( "identity_float_default" ) { 
        std::shared_ptr< identity_float > f( new identity_float );
        fn = std::ref( *f ); 
        any_float_fn = f; 
    }

    any_fn< float >( any_float_fn_ptr any_float_fn, float_fn fn, std::string name ) : any_float_fn( any_float_fn ), fn( fn ), name( name ) {}
};

typedef any_fn< float > any_float_fn;

typedef std::variant <
    // harness functions
    std::shared_ptr< identity_int >,
    std::shared_ptr< adder_int >,

    // ui functions
    std::shared_ptr< slider_int >,
    std::shared_ptr< range_slider_int >,
    std::shared_ptr< menu_int >,
    std::shared_ptr< multi_direction8_picker >,
    std::shared_ptr< custom_blur_picker >
> any_int_fn_ptr;

template<> struct any_fn< int > {
    any_int_fn_ptr any_int_fn;
    int_fn fn;
    std::string name;

    int operator () ( int& val, element_context& context ) { return fn( val, context ); }

    any_fn< int >() : name( "identity_int_default" ) { 
        std::shared_ptr< identity_int > f( new identity_int );
        fn = std::ref( *f ); 
        any_int_fn = f; 
    }

    any_fn< int >( any_int_fn_ptr any_int_fn, int_fn fn, std::string name ) : any_int_fn( any_int_fn ), fn( fn ), name( name ) {}
};

typedef std::variant <
    // harness functions
    std::shared_ptr< identity_interval_float >,
    std::shared_ptr< range_slider_float >
> any_interval_float_fn_ptr;

template<> struct any_fn< interval_float > {
    any_interval_float_fn_ptr any_interval_float_fn;
    interval_float_fn fn;
    std::string name;

    interval_float operator () ( interval_float& val, element_context& context ) { return fn( val, context ); }

    any_fn< interval_float >() : name( "identity_interval_float_default" ) { 
        std::shared_ptr< identity_interval_float > f( new identity_interval_float );
        fn = std::ref( *f ); 
        any_interval_float_fn = f; 
    }

    any_fn< interval_float >( any_interval_float_fn_ptr any_interval_float_fn, interval_float_fn fn, std::string name ) : any_interval_float_fn( any_interval_float_fn ), fn( fn ), name( name ) {}
};

typedef std::variant <
    // harness functions
    std::shared_ptr< identity_interval_int >,
    std::shared_ptr< range_slider_int >
> any_interval_int_fn_ptr;

template<> struct any_fn< interval_int > {
    any_interval_int_fn_ptr any_interval_int_fn;
    interval_int_fn fn;
    std::string name;

    interval_int operator () ( interval_int& val, element_context& context ) { return fn( val, context ); }

    any_fn< interval_int >() : name( "identity_interval_int_default" ) { 
        std::shared_ptr< identity_interval_int > f( new identity_interval_int );
        fn = std::ref( *f ); 
        any_interval_int_fn = f; 
    }

    any_fn< interval_int >( any_interval_int_fn_ptr any_interval_int_fn, interval_int_fn fn, std::string name ) : any_interval_int_fn( any_interval_int_fn ), fn( fn ), name( name ) {}
};

typedef std::variant <
    // harness functions
    std::shared_ptr< identity_vec2f >,
    std::shared_ptr< adder_vec2f >,
    std::shared_ptr< ratio_vec2f >,
    std::shared_ptr< mouse_pos_fn >
> any_vec2f_fn_ptr;

template<> struct any_fn< vec2f > {
    any_vec2f_fn_ptr any_vec2f_fn;
    vec2f_fn fn;
    std::string name;

    vec2f operator () ( vec2f& val, element_context& context ) { return fn( val, context ); }

    any_fn< vec2f >() : name( "identity_vec2f_default" ) { 
        std::shared_ptr< identity_vec2f > f( new identity_vec2f );
        fn = std::ref( *f ); 
        any_vec2f_fn = f; 
    }

    any_fn< vec2f >( any_vec2f_fn_ptr any_vec2f_fn, vec2f_fn fn, std::string name ) : any_vec2f_fn( any_vec2f_fn ), fn( fn ), name( name ) {}
};

typedef std::variant <
    // harness functions
    std::shared_ptr< identity_vec2i >,
    std::shared_ptr< adder_vec2i >,
    std::shared_ptr< mouse_pix_fn >
> any_vec2i_fn_ptr;

template<> struct any_fn< vec2i > {
    any_vec2i_fn_ptr any_vec2i_fn;
    vec2i_fn fn;
    std::string name;

    vec2i operator () ( vec2i& val, element_context& context ) { return fn( val, context ); }

    any_fn< vec2i >() : name( "identity_vec2i_default" ) { 
        std::shared_ptr< identity_vec2i > f( new identity_vec2i );
        fn = std::ref( *f ); 
        any_vec2i_fn = f; 
    }

    any_fn< vec2i >( any_vec2i_fn_ptr any_vec2i_fn, vec2i_fn fn, std::string name ) : any_vec2i_fn( any_vec2i_fn ), fn( fn ), name( name ) {}
};

typedef std::variant <
    // harness functions
    std::shared_ptr< identity_frgb >,
    std::shared_ptr< adder_frgb >
> any_frgb_fn_ptr;

template<> struct any_fn< frgb > {
    any_frgb_fn_ptr any_frgb_fn;
    frgb_fn fn;
    std::string name;

    frgb operator () ( frgb& val, element_context& context ) { return fn( val, context ); }

    any_fn< frgb >() : name( "identity_vec2i_default" ) { 
        std::shared_ptr< identity_frgb > f( new identity_frgb );
        fn = std::ref( *f ); 
        any_frgb_fn = f; 
    }

    any_fn< frgb >( any_frgb_fn_ptr any_frgb_fn, frgb_fn fn, std::string name ) : any_frgb_fn( any_frgb_fn ), fn( fn ), name( name ) {}
};

typedef std::variant <
    // harness functions
    std::shared_ptr< identity_ucolor >,
    std::shared_ptr< adder_ucolor >
> any_ucolor_fn_ptr;

template<> struct any_fn< ucolor > {
    any_ucolor_fn_ptr any_ucolor_fn;
    ucolor_fn fn;
    std::string name;

    ucolor operator () ( ucolor& val, element_context& context ) { return fn( val, context ); }

    any_fn< ucolor >() : name( "identity_ucolor_default" ) { 
        std::shared_ptr< identity_ucolor > f( new identity_ucolor );
        fn = std::ref( *f ); 
        any_ucolor_fn = f; 
    }

    any_fn< ucolor >( any_ucolor_fn_ptr any_ucolor_fn, ucolor_fn fn, std::string name ) : any_ucolor_fn( any_ucolor_fn ), fn( fn ), name( name ) {}
};

typedef std::variant <
    // harness functions
    std::shared_ptr< identity_bb2f >
> any_bb2f_fn_ptr;

template<> struct any_fn< bb2f > {
    any_bb2f_fn_ptr any_bb2f_fn;
    bb2f_fn fn;
    std::string name;

    bb2f operator () ( bb2f& val, element_context& context ) { return fn( val, context ); }

    any_fn< bb2f >() : name( "identity_bb2f_default" ) { 
        std::shared_ptr< identity_bb2f > f( new identity_bb2f );
        fn = std::ref( *f ); 
        any_bb2f_fn = f; 
    }

    any_fn< bb2f >( any_bb2f_fn_ptr any_bb2f_fn, bb2f_fn fn, std::string name ) : any_bb2f_fn( any_bb2f_fn ), fn( fn ), name( name ) {}
};

typedef std::variant <
    // harness functions
    std::shared_ptr< identity_string >,

    // ui functions
    std::shared_ptr< menu_string >
> any_string_fn_ptr;

template<> struct any_fn< std::string > {
    any_string_fn_ptr any_string_fn;
    string_fn fn;
    std::string name;

    std::string operator () ( std::string& val, element_context& context ) { return fn( val, context ); }

    any_fn< std::string >() : name( "identity_string_default" ) { 
        std::shared_ptr< identity_string > f( new identity_string );
        fn = std::ref( *f ); 
        any_string_fn = f; 
    }

    any_fn< std::string >( any_string_fn_ptr any_string_fn, string_fn fn, std::string name ) : any_string_fn( any_string_fn ), fn( fn ), name( name ) {}
};

typedef std::variant <
    // harness functions
    std::shared_ptr< identity_direction4 >,

    // ui functions
    std::shared_ptr< direction_picker_4 >
> any_direction4_fn_ptr;

template<> struct any_fn< direction4 > {
    any_direction4_fn_ptr any_direction4_fn;
    direction4_fn fn;
    std::string name;

    direction4 operator () ( direction4& val, element_context& context ) { return fn( val, context ); }

    any_fn< direction4 >() : name( "identity_direction4_default" ) { 
        std::shared_ptr< identity_direction4 > f( new identity_direction4 );
        fn = std::ref( *f ); 
        any_direction4_fn = f; 
    }

    any_fn< direction4 >( any_direction4_fn_ptr any_direction4_fn, direction4_fn fn, std::string name ) : any_direction4_fn( any_direction4_fn ), fn( fn ), name( name ) {}
};

typedef std::variant <
    // harness functions
    std::shared_ptr< identity_direction8 >,

    // ui functions
    std::shared_ptr< direction_picker_8 >
> any_direction8_fn_ptr;

template<> struct any_fn< direction8 > {
    any_direction8_fn_ptr any_direction8_fn;
    direction8_fn fn;
    std::string name;

    direction8 operator () ( direction8& val, element_context& context ) { return fn( val, context ); }

    any_fn< direction8 >() : name( "identity_direction8_default" ) { 
        std::shared_ptr< identity_direction8 > f( new identity_direction8 );
        fn = std::ref( *f ); 
        any_direction8_fn = f; 
    }

    any_fn< direction8 >( any_direction8_fn_ptr any_direction8_fn, direction8_fn fn, std::string name ) : any_direction8_fn( any_direction8_fn ), fn( fn ), name( name ) {}
};

typedef std::variant < 
    // harness functions
    std::shared_ptr< identity_box_blur_type >,

    // ui functions
    std::shared_ptr< box_blur_picker >
> any_box_blur_type_fn_ptr;

template<> struct any_fn< box_blur_type > {
    any_box_blur_type_fn_ptr any_box_blur_type_fn;
    box_blur_type_fn fn;
    std::string name;

    box_blur_type operator () ( box_blur_type& val, element_context& context ) { return fn( val, context ); }

    any_fn< box_blur_type >() : name( "identity_box_blur_type_default" ) { 
        std::shared_ptr< identity_box_blur_type > f( new identity_box_blur_type );
        fn = std::ref( *f ); 
        any_box_blur_type_fn = f; 
    }

    any_fn< box_blur_type >( any_box_blur_type_fn_ptr any_box_blur_type_fn, box_blur_type_fn fn, std::string name ) : any_box_blur_type_fn( any_box_blur_type_fn ), fn( fn ), name( name ) {}
};

typedef std::variant < 
    // bool harness functions
    std::shared_ptr< initial_element_fn >,
    std::shared_ptr< following_element_fn >,
    std::shared_ptr< top_level_fn >,
    std::shared_ptr< lower_level_fn >,
    std::shared_ptr< random_fn >,
    std::shared_ptr< random_sticky_fn >,

    // equality functions
    std::shared_ptr< equal_float_fn >,
    std::shared_ptr< equal_vec2f_fn >,
    std::shared_ptr< equal_int_fn >,
    std::shared_ptr< equal_vec2i_fn >,
    std::shared_ptr< equal_frgb_fn >,
    std::shared_ptr< equal_ucolor_fn >,
    std::shared_ptr< equal_string_fn >,
    std::shared_ptr< equal_bool_fn >,
    std::shared_ptr< equal_direction4_fn >,
    std::shared_ptr< equal_direction8_fn >,

    // ui functions
    std::shared_ptr< mousedown_fn >,
    std::shared_ptr< mouseover_fn >,
    std::shared_ptr< mouseclick_fn >,
    std::shared_ptr< switch_fn >,
    std::shared_ptr< widget_switch_fn >
> any_bool_fn_ptr;

template<> struct any_fn< bool > {
    any_bool_fn_ptr any_bool_fn;
    bool_fn fn;
    std::string name;

    bool operator () ( bool& val, element_context& context ) { return fn( val, context ); }

    any_fn< bool >() : name( "identity_bool_default" ) { 
        std::shared_ptr< initial_element_fn > f( new initial_element_fn );
        fn = std::ref( *f ); 
        any_bool_fn = f; 
    }

    any_fn< bool >( any_bool_fn_ptr any_bool_fn, bool_fn fn, std::string name ) : any_bool_fn( any_bool_fn ), fn( fn ), name( name ) {}
};

typedef std::variant < 
    std::shared_ptr< initial_element_condition >,
    std::shared_ptr< following_element_condition >,
    std::shared_ptr< top_level_condition >,
    std::shared_ptr< lower_level_condition >,
    std::shared_ptr< random_condition >,
    std::shared_ptr< random_sticky_condition >,

    // equality conditions
    std::shared_ptr< equal_float_condition >,
    std::shared_ptr< equal_vec2f_condition >,
    std::shared_ptr< equal_int_condition >,
    std::shared_ptr< equal_vec2i_condition >,
    std::shared_ptr< equal_frgb_condition >,
    std::shared_ptr< equal_ucolor_condition >,
    std::shared_ptr< equal_string_condition >,
    std::shared_ptr< equal_bool_condition >,
    std::shared_ptr< equal_direction4_condition >,
    std::shared_ptr< equal_direction8_condition >,

    // ui conditions
    std::shared_ptr< mousedown_condition >,
    std::shared_ptr< mouseover_condition >,
    std::shared_ptr< mouseclick_condition >,
    std::shared_ptr< switch_condition >,
    std::shared_ptr< widget_switch_condition >
> any_condition_fn_ptr;

struct any_condition_fn {
    any_condition_fn_ptr my_condition_fn;
    condition_fn fn;
    std::string name;

    bool operator () ( element_context& context ) { return fn( context ); }

    //any_condition_fn() : my_condition_fn( std::shared_ptr< switch_condition >( &switch_static_condition ) ), fn( std::ref( switch_static_condition ) ), name( "switch_static_condition" ) {}
    any_condition_fn() : name( "switch_condition_default" ) {
        std::shared_ptr< switch_condition > f( new switch_condition );
        fn = std::ref( *f ); 
        my_condition_fn = f; }
    any_condition_fn( any_condition_fn_ptr my_condition_fn, condition_fn fn, std::string name ) : my_condition_fn( my_condition_fn ), fn( fn ), name( name ) {}
};

typedef std::variant < 
    // identity function (no effect)
    std::shared_ptr< identity_gen_fn >,

    // single field modifiers
    std::shared_ptr< orientation_gen_fn >,
    std::shared_ptr< scale_gen_fn >,
    std::shared_ptr< rotation_gen_fn >,
    std::shared_ptr< position_gen_fn >,

    // generalized conditional function 
    std::shared_ptr< filter >,

    // generalized functions (alphabetical order)
    std::shared_ptr< advect_element >,
    std::shared_ptr< angle_branch >,
    std::shared_ptr< curly >//,
//    std::shared_ptr< position_list >
> any_gen_fn_ptr;

struct any_gen_fn {
    any_gen_fn_ptr my_gen_fn;
    gen_fn fn;
    std::string name;

    void operator () ( element_context& context ) { fn( context ); }

    any_gen_fn() : name( "identity_gen_default" ) { 
        std::shared_ptr< identity_gen_fn > f( new identity_gen_fn );
        fn = std::ref( *f ); 
        my_gen_fn = f; }
    any_gen_fn( any_gen_fn_ptr my_gen_fn, gen_fn fn, std::string name ) : my_gen_fn( my_gen_fn ), fn( fn ), name( name ) {}
};

typedef std::variant < 
    any_fn< float >,
    any_fn< int >,
    any_fn< interval_float >,
    any_fn< interval_int >,
    any_fn< vec2f >,
    any_fn< vec2i >,
    any_fn< frgb >,
    any_fn< ucolor >,
    any_fn< bb2f >,
    any_fn< bb2i >,
    any_fn< std::string >,
    any_fn< direction4 >,
    any_fn< direction8 >,
    any_fn< box_blur_type >,
    any_fn< bool >,
    any_condition_fn,
    any_gen_fn          
> any_function;

/*
template< class T > gen_fn   to_gen_fn(   std::shared_ptr< T >& ptr );
gen_fn resolve_gen_fn( any_gen_fn_ptr& any_fn_ptr);

template< class T > float_fn to_float_fn( std::shared_ptr< T > ptr );
float_fn resolve_float_fn( any_float_fn_ptr any_fn_ptr);

template< class T > int_fn   to_int_fn(   std::shared_ptr< T > ptr );
int_fn resolve_int_fn( any_int_fn_ptr any_fn_ptr);

template< class T > vec2f_fn to_vec2f_fn( std::shared_ptr< T > ptr );
vec2f_fn resolve_vec2f_fn( any_vec2f_fn_ptr any_fn_ptr);

template< class T > vec2i_fn to_vec2i_fn( std::shared_ptr< T > ptr );
vec2i_fn resolve_vec2i_fn( any_vec2i_fn_ptr any_fn_ptr);
*/
#endif // __ANY_FUNCTION_HPP
