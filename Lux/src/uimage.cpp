#include "uimage.hpp"
#include <memory>
#include "image_loader.hpp"

// pixel modification functions
void uimage::grayscale() {
    for( auto& f : base ) { f = gray( f ); }
    mip_it();
}

void uimage::load( const std::string& filename ) {
    reset();
    image_loader loader( filename );
    set_dim( { loader.xsiz, loader.ysiz } );

    ucolor f = 0xff000000;

    for (auto it = std::begin( loader.img ); it <= std::end( loader.img ); ) {
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

        if( loader.channels == 4 ) 
        {
            setac( f, *it );
            it++;
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
        base.push_back( f );
    }
    mip_it();
}

void uimage::spool( std::vector< unsigned char >& img )
{
    for( auto& f : base ) {
        img.push_back( rc( f ) );
        img.push_back( gc( f ) );
        img.push_back( bc( f ) ); 
    }
}

void uimage::write_jpg( const std::string& filename, int quality ) {
    std::vector< unsigned char > img;
	spool( img );
	wrapped_write_jpg( filename.c_str(), dim.x, dim.y, 3, img.data(), quality );
}

void uimage::write_png( const std::string& filename ) {
	wrapped_write_png( filename.c_str(), dim.x, dim.y, 4, (unsigned char *)base.data() );
}

void uimage::write_file(const std::string &filename, file_type type, int quality ) {
    switch( type ) {
        case FILE_JPG: write_jpg( filename, quality ); break;
        case FILE_PNG: write_png( filename ); break;
        case FILE_BINARY: write_binary( filename ); break;
        default: std::cout << "fimage::write_file: unknown file type " << type << std::endl;
    }
}