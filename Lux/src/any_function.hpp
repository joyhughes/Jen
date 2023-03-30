#ifndef __ANY_FUNCTION_HPP
#define __ANY_FUNCTION_HPP

#include <variant>
#include "next_element.hpp"

template < class T > struct any_fn {};

typedef std::variant < 
    // harness functions
    std::shared_ptr< identity_float >,
    std::shared_ptr< adder_float >,
    std::shared_ptr< log_fn >,
    std::shared_ptr< ratio_float >,
    std::shared_ptr< wiggle >,

    // parameter functions
    std::shared_ptr< index_param< float > >,
    std::shared_ptr< scale_param< float > >,
    std::shared_ptr< time_param<  float > >

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
    std::shared_ptr< adder_int >
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
    std::shared_ptr< identity_vec2f >,
    std::shared_ptr< adder_vec2f >,
    std::shared_ptr< ratio_vec2f >
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
    std::shared_ptr< adder_vec2i >
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
    std::shared_ptr< switch_condition >,
    std::shared_ptr< initial_element_condition >,
    std::shared_ptr< following_element_condition >,
    std::shared_ptr< top_level_condition >,
    std::shared_ptr< lower_level_condition >,
    std::shared_ptr< random_condition >,
    std::shared_ptr< random_sticky_condition >
> any_condition_fn_ptr;

struct any_condition_fn {
    any_condition_fn_ptr my_condition_fn;
    bool_fn fn;
    std::string name;

    bool operator () ( bool& val, element_context& context ) { return fn( val, context ); }

    //any_condition_fn() : my_condition_fn( std::shared_ptr< switch_condition >( &switch_static_condition ) ), fn( std::ref( switch_static_condition ) ), name( "switch_static_condition" ) {}
    any_condition_fn() : name( "switch_condition_default" ) { 
        std::shared_ptr< switch_condition > f( new switch_condition );
        fn = std::ref( *f ); 
        my_condition_fn = f; }
    any_condition_fn( any_condition_fn_ptr my_condition_fn, bool_fn fn, std::string name ) : my_condition_fn( my_condition_fn ), fn( fn ), name( name ) {}
};

typedef std::variant < 
    // identity function (no effect)
    std::shared_ptr< identity_gen_fn >,

    // single field modifiers
    std::shared_ptr< orientation_gen_fn >,
    std::shared_ptr< scale_gen_fn >,

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

    bool operator () ( element_context& context ) { return fn( context ); }

    any_gen_fn() : name( "identity_gen_default" ) { 
        std::shared_ptr< identity_gen_fn > f( new identity_gen_fn );
        fn = std::ref( *f ); 
        my_gen_fn = f; }
    any_gen_fn( any_gen_fn_ptr my_gen_fn, gen_fn fn, std::string name ) : my_gen_fn( my_gen_fn ), fn( fn ), name( name ) {}
};

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
