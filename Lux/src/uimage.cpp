#include "uimage.hpp"
#include <memory>
#include "image_loader.hpp"


// pixel modification functions
void uimage::grayscale() {
    for( auto& f : base ) { f = gray( f ); }
    mip_it();
}

template<> void image< ucolor >::load( const std::string& filename ) {
    std::cout << "uimage::load(): " << filename << std::endl;
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

template<> void image< ucolor >::write_jpg( const std::string& filename, int quality ) {
    std::cout << "uimage::write_jpg: " << filename << std::endl;
    std::vector< unsigned char > carray;
    for( auto& f : base ) {
        carray.push_back( rc( f ) );
        carray.push_back( gc( f ) );
        carray.push_back( bc( f ) ); 
    }
	wrapped_write_jpg( filename.c_str(), dim.x, dim.y, 3, carray.data(), quality );
}

template<> void image< ucolor >::write_png( const std::string& filename ) {
	wrapped_write_png( filename.c_str(), dim.x, dim.y, 4, (unsigned char *)base.data() );
}

template<> void image< ucolor >::write_file(const std::string &filename, file_type type, int quality ) {
    std::cout << "uimage::write_file: " << filename << std::endl;
    switch( type ) {
        case FILE_JPG: write_jpg( filename, quality ); break;
        case FILE_PNG: write_png( filename ); break;
        case FILE_BINARY: write_binary( filename ); break;
        default: std::cout << "fimage::write_file: unknown file type " << type << std::endl;
    }
}

uimage::uimage(const std::__1::string &filename) : image<ucolor>() {
    load(filename);
}
