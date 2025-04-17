#include "emscripten_utils.hpp"

#ifdef __EMSCRIPTEN__
#include <sstream>
#include <iomanip>
#include <emscripten.h> // Include Emscripten header here
#include <algorithm>    // For string find/replace

std::string escape_for_js_string(const std::string& input) {
    std::string output = input;
    // Replace backslashes first
    size_t start_pos = 0;
    while((start_pos = output.find('\\', start_pos)) != std::string::npos) {
        output.replace(start_pos, 1, "\\\\");
        start_pos += 2;
    }
    // Replace single quotes
    start_pos = 0;
    while((start_pos = output.find('\'', start_pos)) != std::string::npos) {
        output.replace(start_pos, 1, "\\'");
        start_pos += 2;
    }
    // Replace double quotes (optional, but safer)
    start_pos = 0;
    while((start_pos = output.find('"', start_pos)) != std::string::npos) {
        output.replace(start_pos, 1, "\\\"");
        start_pos += 2;
    }
    // Replace newlines (optional)
    start_pos = 0;
    while((start_pos = output.find('\n', start_pos)) != std::string::npos) {
        output.replace(start_pos, 1, "\\n");
        start_pos += 2;
    }
    return output;
}

std::string ucolor_to_hex_string(ucolor color) {
    std::ostringstream oss;
    oss << "0x"
        << std::hex
        << std::setfill('0')
        << std::setw(sizeof(ucolor) * 2)
        << static_cast<unsigned long long>(color);
    return oss.str();
}

void emscripten_message( std::string msg ) {
    std::string escaped_msg = escape_for_js_string(msg);
    std::string js_command = "console.log('" + escaped_msg + "');";
    emscripten_run_script( js_command.c_str() );
}

void emscripten_error( std::string msg ) {
    emscripten_message( msg );
    // Consider if exit(0) is always appropriate for ERROR
    exit( 0 );
}

#endif // __EMSCRIPTEN__
