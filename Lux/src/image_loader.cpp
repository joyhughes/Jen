// image_loader wraps low level image I/O routines. Implementation can be changed without affecting image classes.
// Using stb_image, C-style pointer is created when class is initialized with a file name, and freed in the destructor.
// Copy constructor and operator= are disabled to prevent segmentation faults or memory leaks

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb_image/stb_image_write.h"
#include "image_loader.hpp"
#include <iostream>
#include <stdexcept>

image_loader :: image_loader( const std::string& filename ) : stbi_ptr( stbi_load( filename.c_str(), &xsiz, &ysiz, &channels, 0 ) ) {
    if(!stbi_ptr) 
        throw std::runtime_error (std::string( "Image load error: \nFile" ) + filename + "\nReason:" + stbi_failure_reason() + "\n" ); 
    img = std::span< unsigned char >( stbi_ptr, xsiz * ysiz * channels );
} 

image_loader :: ~image_loader() {
    if( stbi_ptr) stbi_image_free( stbi_ptr );
}

void wrapped_write_jpg( const std::string& filename, const int xdim, const int ydim, const int channels, const unsigned char* data, const int quality ) {
	int ok = stbi_write_jpg( filename.c_str(), xdim, ydim, channels, data, quality );
    if( !ok ) 
        throw std::runtime_error (std::string( "Write jpeg error: \nFile" ) + filename + "\nReason:" + stbi_failure_reason() + "\n" ); 
}

void wrapped_write_png( const std::string& filename, const int xdim, const int ydim, const int channels, const unsigned char* data ) {
	int ok = stbi_write_png( filename.c_str(), xdim, ydim, channels, data, xdim * channels );
    if( !ok ) 
        throw std::runtime_error (std::string( "Write jpeg error: \nFile" ) + filename + "\nReason:" + stbi_failure_reason() + "\n" ); 
}
