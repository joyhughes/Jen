#ifndef EMSCRIPTEN_UTILS_HPP
#define EMSCRIPTEN_UTILS_HPP

#include "string"
#include "ucolor.hpp"


std::string escape_for_js_string(const std::string& input);

// Declarations for message/error wrappers
void emscripten_message( std::string msg );
void emscripten_error( std::string msg );

std::string ucolor_to_hex_string(ucolor color);

// Define macros here for consistency, guarded by __EMSCRIPTEN__
#define DEBUG( msg ) emscripten_message( msg )
#define ERROR( msg ) emscripten_error( msg )

#else

#include <iostream>
#include <stdexcept>
#define DEBUG( msg ) { std::string debug_msg = msg; std::cout << debug_msg << std::endl; }
#define ERROR( msg ) throw std::runtime_error( msg )

#endif
