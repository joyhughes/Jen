// Floating point image class using image template as a base

#include "fimage.hpp"
#include <memory>
#include "image_loader.hpp"

// pixel modification functions

void fimage::clamp( float minc, float maxc ) {
    for( auto& c : base ) { linalg::clamp( c, minc, maxc ); }
    mip_it();
}

void fimage::constrain() {
    for( auto& c : base ) { ::constrain( c ); }
    mip_it();
}

void fimage::grayscale() {
    for( auto& c : base ) { c = gray( c ); }
    mip_it();
}

void fimage::load( const std::string& filename ) {
    std::cout << "fimage::load\n";
    reset();
    image_loader loader( filename );
    set_dim( { loader.xsiz, loader.ysiz } );

	int c = 0;
    frgb f;

    for (auto it = std::begin (loader.img); it <= std::end (loader.img); ) {
        if( loader.channels == 1 )	// monochrome image
        {
            setrc( f, *it );
            setgc( f, *it );
            setbc( f, *it );
            it++;
        }

        if( loader.channels == 2 ) // don't know what to do in this case
        {
            setrc( f, *it );
            setgc( f, *it );
            setbc( f, *it );
            it++; it++;
        }

        if( loader.channels == 4 ) 	it++;   // argb format

        if( ( loader.channels == 3 ) | ( loader.channels == 4 ) )
        {
            setrc( f, *it );
            it++;
            setgc( f, *it );
            it++;
            setbc( f, *it );
            it++;
        }

        base.push_back( f );
    }
    mip_it();
    std::cout << "Image load complete\n";
}

void fimage::quantize( std::vector< unsigned char >& img )
{
    for( auto& f : base ) {
        img.push_back( rc( f ) );
        img.push_back( gc( f ) );
        img.push_back( bc( f ) ); }
}

void fimage::write_jpg( const std::string& filename, int quality ) {
    std::vector< unsigned char > img;
	quantize( img );
	wrapped_write_jpg( filename.c_str(), dim.x, dim.y, 3, img.data(), quality );
}

void fimage::write_png( const std::string& filename ) {    
    std::vector< unsigned char > img;
	quantize( img );
	wrapped_write_png( filename.c_str(), dim.x, dim.y, 3, img.data() );
}