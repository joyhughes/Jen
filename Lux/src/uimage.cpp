#include "uimage.hpp"
#include <memory>
#include "image_loader.hpp"


// pixel modification functions
template<> void uimage::grayscale() {
    auto& base = mip[ 0 ];
    for( auto& f : base ) { f = gray( f ); }
    mip_utd = false;
}

template<> void uimage::invert() {
    auto& base = mip[ 0 ];
    for( auto& c : base ) { ::invert( c ); }
    mip_utd = false;
}

template<> void uimage::rotate_components( const int& r ) {
    auto& base = mip[ 0 ];
    for( auto& c : base ) { ::rotate_components( c, r ); }
    mip_utd = false;
}

template<> void uimage::rgb_to_hsv() {
    for( auto& level : mip ) {
        for( auto& c : level ) { c = ::rgb_to_hsv( c ); }
    }
}

template<> void uimage::hsv_to_rgb() {
    auto& base = mip[ 0 ];
    for( auto& c : base ) { c = ::hsv_to_rgb( c ); }
    mip_utd = false;
}

template<> void uimage::rotate_hue( const float& h ) {
    auto& base = mip[ 0 ];
    unsigned int r = h * 256.0f / 360.0f;
    r <<= 16;
    for( auto& c : base ) { c = ::rotate_hue( c, r ); }
    mip_utd = false;
}

template<> void uimage::bit_plane( const ucolor& q ) {
    auto& base = mip[ 0 ];
    for( auto& c : base ) { c = ::bit_plane( c, q ); }
    mip_utd = false;
}

template<> void uimage::load( const std::string& filename ) {
    reset();
    image_loader loader( filename );
    dim = { loader.xsiz, loader.ysiz };
    refresh_bounds();
    //std::cout << "uimage::load: " << filename << " " << loader.xsiz << " " << loader.ysiz << " " << loader.channels << std::endl;
    ucolor f = 0xff000000;
    auto& base = mip[0];
    base.reserve( dim.x * dim.y );

//    int size = loader.xsiz * loader.ysiz;
//    auto it = std::begin( loader.img );
    for (auto it = std::begin( loader.img ); it < std::end( loader.img ); ) {
 //   for( int i = 0; i < size; i++ ) {
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
    // default mip mapping for testing - future: set use_mip from scene file
    //use_mip(true);
    //mip_it();
}

template<> void uimage::write_jpg( const std::string& filename, int quality, int level ) {
    std::vector< unsigned char > carray;
    carray.reserve( dim.x * dim.y * 3 );
    auto& pixels = mip[ level ];
    for( auto& f : pixels ) {
        carray.push_back( bc( f ) );
        carray.push_back( gc( f ) );
        carray.push_back( rc( f ) ); 
    }
	wrapped_write_jpg( filename.c_str(), mip_dim[level].x, mip_dim[level].y, 3, carray.data(), quality );
}

template<> void uimage::write_png( const std::string& filename, int level ) {
    auto& pixels = mip[ level ];
	wrapped_write_png( filename.c_str(), mip_dim[level].x, mip_dim[level].y, 4, (unsigned char *)pixels.data() );
}

// Fixed point version of sample

template<> const ucolor image< ucolor >::sample ( const unsigned int& mip_level, const unsigned int& mip_blend, const vec2i& vi ) const  {
    //std::cout << "sample ucolor" << std::endl;
    //std::cout << "mip_dim.size = " << mip_dim.size() << std::endl;
    int l_index = ( vi.x >> ( 16 + mip_level     ) ) + ( vi.y >> ( 16 + mip_level     ) ) * mip_dim[ mip_level     ].x;
    int u_index = ( vi.x >> ( 16 + mip_level + 1 ) ) + ( vi.y >> ( 16 + mip_level + 1 ) ) * mip_dim[ mip_level + 1 ].x;
    //std::cout << "uimage sample - l_index: " << l_index << " u_index: " << u_index << std::endl;
    return  blend(
                blend(
                    blend( mip[ mip_level ][ l_index + mip_dim[ mip_level ].x + 1 ], mip[ mip_level ][ l_index + mip_dim[ mip_level ].x ], ( vi.x >> ( 8 + mip_level ) ) & 0xff ),
                    blend( mip[ mip_level ][ l_index                          + 1 ], mip[ mip_level ][ l_index                          ], ( vi.x >> ( 8 + mip_level ) ) & 0xff ),
                    ( vi.y >> ( 8 + mip_level ) ) & 0xff 
                ),
                blend(
                    blend( mip[ mip_level + 1 ][ u_index + mip_dim[ mip_level + 1 ].x + 1 ], mip[ mip_level + 1 ][ u_index + mip_dim[ mip_level + 1 ].x ], ( vi.x >> ( 8 + mip_level + 1 ) ) & 0xff ),
                    blend( mip[ mip_level + 1 ][ u_index                              + 1 ], mip[ mip_level + 1 ][ u_index ],                              ( vi.x >> ( 8 + mip_level + 1 ) ) & 0xff ),
                    0xff - ( vi.y >> ( 8 + mip_level + 1 ) ) & 0xff 
                ),
                mip_blend >> 8
            );
}

template<> void uimage::dump() {
    auto& base = mip[ 0 ];
    for( auto& v : base ) { std::cout << std::hex << v; }
}
