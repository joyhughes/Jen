// Floating point image class using image template as a base

#include "fimage.hpp"
#include <memory>
#include "image_loader.hpp"

// pixel modification functions
void fimage :: grayscale() {
    std :: transform( base.begin(), base.end(), base.begin(),  []( frgb &f ) { return gray( f ); } );
}

void fimage :: load( const std :: string& filename ) {
    reset();
    image_loader loader( filename );
    set_dim( { loader.xsiz, loader.ysiz } );

	int c = 0;
    frgb f;

    for (auto it = begin (loader.img); it <= end (loader.img); ) {
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

        if( ( loader.channels == 3 ) | ( loader.channels == 4 ) )
        {
            setrc( f, *it );
            it++;
            setgc( f, *it );
            it++;
            setbc( f, *it );
            it++;
        }

        // skip alpha channel - rgba ... if argb need to move line up
        if( loader.channels == 4 ) it++;	
        base.push_back( f );
    }
    mip_it();
}

void fimage :: quantize( std :: vector< unsigned char >& img )
{
    std :: for_each( base.begin(), base.end(), [ &img ]( const frgb &f ) {
        img.push_back( rc( f ) );
        img.push_back( gc( f ) );
        img.push_back( bc( f ) ); } );
}

void fimage :: write_jpg( const std :: string& filename, int quality ) {
    std :: vector< unsigned char > img;
	quantize( img );
	wrapped_write_jpg( filename.c_str(), dim.x, dim.y, 3, img.data(), quality );
}

void fimage :: write_png( const std :: string& filename ) {
}