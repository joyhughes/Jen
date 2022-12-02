#include "vector_field.hpp"
#include "fimage.hpp"
#include "uimage.hpp"

// simple conversion and manipulation of image binary files

void to_frgb_file( std::string& in_filename, std::string& out_filename ) {
    fimage img( in_filename );
    img.write_binary( out_filename );
}

void to_jpg_file( std::string& in_filename, std::string& out_filename ) {
    fimage img;
    img.read_binary( in_filename );
    img.write_jpg( out_filename, 100 );
}

void subtract_image( std::string& file1, std::string& file2, std::string& basename ) {
    fimage img1( file1 ), img2( file2 );
    img1 -= img2;
    std::string bin_name = basename + ".frgb";
    img1.write_binary( bin_name );
    std::string pos_name = basename + "_pos.jpg";
    img1.write_jpg( pos_name, 100 );
    img1 *= -1.0f;
    std::string neg_name = basename + "_neg.jpg";
    img1.write_jpg( neg_name, 100 );
}

int main( int argc, char** argv ) {
    if( argc < 3 ) {
        std::cout << "Usage: ./sploot file_in file_out\n./sploot -fromBinary file_in file_out\n./sploot -subtract file1 file2 basename\n";
        return 0;
    }
    if( std::string( argv[ 1 ]) == "-fromBinary" ) {
        std::string in_name(  argv[ 2 ] );
        std::string out_name( argv[ 3 ] );
        to_jpg_file( in_name, out_name );
    } else if( std::string( argv[ 1 ]) == "-subtract" ) {
        std::string file1(  argv[ 2 ] );
        std::string file2( argv[ 3 ] );
        std::string basename( argv[ 4 ] );      
        subtract_image( file1, file2, basename );
    } else {
        std::string in_name(  argv[ 1 ] );
        std::string out_name( argv[ 2 ] );
        to_frgb_file( in_name, out_name );
    }
    return 0;
}