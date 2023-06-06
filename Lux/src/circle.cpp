#include "image.hpp"
// Creates a unit circle and saves it as JPG and PNG files.

int main(int argc, char**argv ) {
    image< ucolor > img( vec2i{ 512, 512 } );
    img.fill( 0xffffffff );
    img.crop_circle();
    img.write_jpg( "../samples/circle.jpg", 100 );
    img.write_png( "../samples/circle.png" );
    return 0;
}