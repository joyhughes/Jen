#include "any_function.hpp"
#include "scene.hpp"
#include "next_element.hpp"

bool_function_condition::bool_function_condition(const std::string& source_name) :
    source_function_name(source_name) {}

bool bool_function_condition::operator()(element_context &context) {
    if (source_function_name.empty()) {
        std::cerr << "Warning: bool_function_condition called without a source_function_name set." << std::endl;
        return false;
    }
    // Compiler now has full definitions of element_context, scene, and any_function
    if (context.s.functions.count(source_function_name)) {
        any_function &source_func_variant = context.s.functions[source_function_name];
        if (std::holds_alternative<any_fn<bool> >(source_func_variant)) {
            any_fn<bool> &bool_fn_wrapper = std::get<any_fn<bool> >(source_func_variant);
            bool dummy_val = false;
            return bool_fn_wrapper(dummy_val, context);
        } else {
            std::cerr << "Warning: Source function '" << source_function_name
                    << "' for bool_function_condition is not type any_fn<bool>." << std::endl;
            return false;
        }
    } else {
        std::cerr << "Warning: Source function '" << source_function_name
                << "' for bool_function_condition not found in scene." << std::endl;
        return false;
    }
}


/*
template< class T > gen_fn to_gen_fn( std::shared_ptr< T >& ptr ) { return *ptr; }
gen_fn resolve_gen_fn( any_gen_fn_ptr& any_fn_ptr) {
    auto fn = std::visit([](auto&& fn_ptr ) { return to_gen_fn( fn_ptr ); }, any_fn_ptr );
    return std::ref(fn);
}

template< class T > float_fn to_float_fn( std::shared_ptr< T > ptr ) { return *ptr; }
float_fn resolve_float_fn( any_float_fn_ptr any_fn_ptr) {
    auto fn =std::visit([](auto&& fn_ptr ) { return to_float_fn( fn_ptr ); }, any_fn_ptr );
    return fn;
}

template< class T > int_fn to_int_fn( std::shared_ptr< T > ptr ) { return *ptr; }
int_fn resolve_int_fn( any_int_fn_ptr& any_fn_ptr) {
    auto fn = std::visit([](auto&& fn_ptr ) { return to_int_fn( fn_ptr ); }, any_fn_ptr );
    return fn;
}

template< class T > vec2f_fn to_vec2f_fn( std::shared_ptr< T > ptr ) { return *ptr; }
vec2f_fn resolve_vec2f_fn( any_vec2f_fn_ptr& any_fn_ptr) {
    auto fn = std::visit([](auto&& fn_ptr ) { return to_vec2f_fn( fn_ptr ); }, any_fn_ptr );
    return fn;
}

template< class T > vec2i_fn to_vec2i_fn( std::shared_ptr< T > ptr ) { return *ptr; }
vec2i_fn resolve_vec2i_fn( any_vec2i_fn_ptr any_fn_ptr) {
    auto fn = std::visit([](auto&& fn_ptr ) { return to_vec2i_fn( fn_ptr ); }, any_fn_ptr );
    return fn;
}
*/
