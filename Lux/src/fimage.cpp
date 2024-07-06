// Floating point image class using image template as a base

#include "fimage.hpp"
#include <memory>
#include "image_loader.hpp"

// pixel modification functions

template<> void fimage::clamp( float minc, float maxc ) {
    for( auto& c : base ) { linalg::clamp( c, minc, maxc ); }
    //mip_it();
}

template<> void fimage::constrain() {
    for( auto& c : base ) { ::constrain( c ); }
    //mip_it();
}

template<> void fimage::grayscale() {
    for( auto& c : base ) { c = gray( c ); }
    //mip_it();
}

template<> void fimage::invert() {
    for( auto& c : base ) { ::invert( c ); }
    //mip_it();
}

template<> void fimage::rotate_colors( const int& r ) {
    for( auto& c : base ) { rotate_color( c, r ); }
    //mip_it();
}

template<> void fimage::rgb_to_hsv() {
    for( auto& c : base ) { c = ::rgb_to_hsv( c ); }
    // rather than calling mip_it(), probably better to calculate hsv at each mip level
}

template<> void fimage::hsv_to_rgb() {
    for( auto& c : base ) { c = ::hsv_to_rgb( c ); }
    //mip_it();
}

template<> void fimage::load( const std::string& filename ) {
    //std::cout << "fimage::load " << filename << std::endl;
    reset();
    image_loader loader( filename );
    dim = { loader.xsiz, loader.ysiz };
    refresh_bounds();
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
    //mip_it();
    //std::cout << "Image load complete\n";
}

template<> void fimage::write_jpg( const std::string& filename, int quality ) {
    std::vector< unsigned char > img;
    for( auto& f : base ) {
        img.push_back( rc( f ) );
        img.push_back( gc( f ) );
        img.push_back( bc( f ) ); 
    }
    wrapped_write_jpg( filename.c_str(), dim.x, dim.y, 3, img.data(), quality );
}

template<> void fimage::write_png( const std::string& filename ) {    
    std::vector< unsigned char > img;
    for( auto& f : base ) {
        img.push_back( rc( f ) );
        img.push_back( gc( f ) );
        img.push_back( bc( f ) ); 
    }
	wrapped_write_png( filename.c_str(), dim.x, dim.y, 3, img.data() );
}

