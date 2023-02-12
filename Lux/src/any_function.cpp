#include "any_function.hpp"

template< class T > gen_fn to_gen_fn( std::shared_ptr< T > ptr ) { return *ptr; }
gen_fn resolve_gen_fn( any_gen_fn_ptr any_fn_ptr) {
    auto fn = std::visit([](auto&& fn_ptr ) { return to_gen_fn( fn_ptr ); }, any_fn_ptr );
    return fn;
}

template< class T > float_fn to_float_fn( std::shared_ptr< T > ptr ) { return *ptr; }
float_fn resolve_float_fn( any_float_fn_ptr any_fn_ptr) {
    auto fn =std::visit([](auto&& fn_ptr ) { return to_float_fn( fn_ptr ); }, any_fn_ptr );
    return fn;
}

template< class T > int_fn to_int_fn( std::shared_ptr< T > ptr ) { return *ptr; }
int_fn resolve_int_fn( any_int_fn_ptr any_fn_ptr) {
    auto fn = std::visit([](auto&& fn_ptr ) { return to_int_fn( fn_ptr ); }, any_fn_ptr );
    return fn;
}

template< class T > vec2f_fn to_vec2f_fn( std::shared_ptr< T > ptr ) { return *ptr; }
vec2f_fn resolve_vec2f_fn( any_vec2f_fn_ptr any_fn_ptr) {
    auto fn = std::visit([](auto&& fn_ptr ) { return to_vec2f_fn( fn_ptr ); }, any_fn_ptr );
    return fn;
}

template< class T > vec2i_fn to_vec2i_fn( std::shared_ptr< T > ptr ) { return *ptr; }
vec2i_fn resolve_vec2i_fn( any_vec2i_fn_ptr any_fn_ptr) {
    auto fn = std::visit([](auto&& fn_ptr ) { return to_vec2i_fn( fn_ptr ); }, any_fn_ptr );
    return fn;
}