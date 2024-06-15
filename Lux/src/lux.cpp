#include <iostream>
#include <optional>
#include "linalg.h"
#include "vect2.hpp"
#include "vector_field.hpp"
#include "frgb.hpp"
#include "fimage.hpp"
#include "ucolor.hpp"
#include "uimage.hpp"
#include "scene.hpp"
#include "warp.hpp"
#include "life.hpp"

#include <sstream>
#include <string>
#include <iomanip>

void render( std::string scene_filename, std::string file_out, vec2i dim = { 512, 512 } ) {
    scene s( scene_filename );
    //std::cout << "Scene object created" << std::endl;
    s.render_and_save( file_out, dim );
    //std::cout << "Render complete " << file_out << std::endl;
}

void animate( std::string scene_filename, std::string basename, int nframes ) {
    //std::cout << "scene::animate" << std::endl;
    scene s( scene_filename );
    //std::cout << "Scene object created" << std::endl;
    s.animate( basename, nframes );
    //std::cout << "Render complete " << basename << std::endl;
}

int main( int argc, char** argv ) {
    if( argc < 3 ) {
        std::cout << "Usage: ./lux file_in file_out [nframes]\n";
        return 0;
    }
    std::string scene_filename(  argv[ 1 ] );
    std::string output_name( argv[ 2 ] );
    
    if( argc == 3 ) render( scene_filename, output_name /*, { 3648, 3648 } */);
    else {
        int nframes;
        std::stringstream ss( argv[ 3 ] );
        ss >> nframes; 
        animate( scene_filename, output_name, nframes );
    }
    
    //std::cout << "Done!" << std::endl;
    return 0;
} 
