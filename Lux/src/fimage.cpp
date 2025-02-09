// Floating point image class using image template as a base

#include "fimage.hpp"
#include <memory>
#include "image_loader.hpp"

// pixel modification functions

template<> void fimage::clamp( float minc, float maxc ) {
    auto& base = mip[ 0 ];
    mip_utd = false;
    for( auto& c : base ) { linalg::clamp( c, minc, maxc ); }
}

template<> void fimage::constrain() {
    auto& base = mip[ 0 ];
    mip_utd = false;
    for( auto& c : base ) { ::constrain( c ); }
}

template<> void fimage::grayscale() {
    auto& base = mip[ 0 ];
    mip_utd = false;
    for( auto& c : base ) { c = gray( c ); }
}

template<> void fimage::invert() {
    auto& base = mip[ 0 ];
    mip_utd = false;
    for( auto& c : base ) { ::invert( c ); }
}

template<> void fimage::rotate_components( const int& r ) {
    auto& base = mip[ 0 ];
    mip_utd = false;
    for( auto& c : base ) { ::rotate_components( c, r ); }
}

template<> void fimage::rgb_to_hsv() {
    mip_it();
    for( auto& level : mip ) {
        { 
            for( auto& c : level ) { c = ::rgb_to_hsv( c ); }
        }
    }
}

template<> void fimage::hsv_to_rgb() {
    auto& base = mip[ 0 ];
    mip_utd = false;
    for( auto& c : base ) { c = ::hsv_to_rgb( c ); }
}

template<> void fimage::rotate_hue( const float& h ) {
    mip_it();
    for( auto& level : mip ) {
        { 
            for( auto& c : level ) { c = ::rotate_hue( c, h ); }
        }
    }
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
    auto& base = mip[ 0 ];    

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

template<> void fimage::write_jpg( const std::string& filename, int quality, int level ) {
    std::vector< unsigned char > img;
    img.reserve( dim.x * dim.y * 3 );
    auto& pixels = mip[ level ];

    for( auto& f : pixels ) {
        img.push_back( rc( f ) );
        img.push_back( gc( f ) );
        img.push_back( bc( f ) ); 
    }
    wrapped_write_jpg( filename.c_str(), mip_dim[level].x, mip_dim[level].y, 3, img.data(), quality );
}

template<> void fimage::write_png( const std::string& filename, int level ) {    
    std::vector< unsigned char > img;
    img.reserve( dim.x * dim.y * 3 );
    auto& pixels = mip[ level ];

    for( auto& f : pixels ) {
        img.push_back( rc( f ) );
        img.push_back( gc( f ) );
        img.push_back( bc( f ) ); 
    }
	wrapped_write_png( filename.c_str(), mip_dim[level].x, mip_dim[level].y, 3, img.data() );
}
