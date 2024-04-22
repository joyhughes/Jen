#include "uimage.hpp"
#include <memory>
#include "image_loader.hpp"


// pixel modification functions
template<> void uimage::grayscale() {
    for( auto& f : base ) { f = gray( f ); }
    //mip_it();
}

template<> void uimage::invert() {
    for( auto& c : base ) { ::invert( c ); }
    //mip_it();
}

template<> void uimage::rotate_colors( const int& r ) {
    for( auto& c : base ) { rotate_color( c, r ); }
    //mip_it();
}

template<> void uimage::load( const std::string& filename ) {
    base.clear();
    image_loader loader( filename );
    dim = { loader.xsiz, loader.ysiz };
    refresh_bounds();
    //std::cout << "uimage::load: " << filename << " " << loader.xsiz << " " << loader.ysiz << " " << loader.channels << std::endl;
    ucolor f = 0xff000000;

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
    //mip_it();
}

template<> void uimage::write_jpg( const std::string& filename, int quality ) {
    //std::cout << "uimage::write_jpg: " << filename << std::endl;
    //dump();
    std::vector< unsigned char > carray;
    for( auto& f : base ) {
        carray.push_back( rc( f ) );
        carray.push_back( gc( f ) );
        carray.push_back( bc( f ) ); 
    }
	wrapped_write_jpg( filename.c_str(), dim.x, dim.y, 3, carray.data(), quality );
}

template<> void uimage::write_png( const std::string& filename ) {
	wrapped_write_png( filename.c_str(), dim.x, dim.y, 4, (unsigned char *)base.data() );
}

// Fixed point version of sample

template<> const ucolor image< ucolor >::sample ( const unsigned int mip_level, const unsigned int mip_blend, const vec2i& vi ) const  {
    int l_index = ( vi.x >> ( 16 + mip_level     ) ) + ( vi.y >> ( 16 + mip_level     ) ) * mip_dim[ mip_level     ].x;
    int u_index = ( vi.x >> ( 16 + mip_level + 1 ) ) + ( vi.y >> ( 16 + mip_level + 1 ) ) * mip_dim[ mip_level + 1 ].x;

    return  blend(
                blend(
                    blend( mip[ mip_level ][ l_index ], mip[ mip_level ][ l_index + 1 ],( vi.x >> ( 8 + mip_level ) ) & 0xff ),
                    blend( mip[ mip_level ][ l_index + mip_dim[ mip_level ].x ], mip[ mip_level ][ l_index + mip_dim[ mip_level ].x + 1 ],( vi.x >> ( 8 + mip_level ) ) & 0xff ),
                    ( vi.y >> ( 8 + mip_level ) ) & 0xff 
                ),
                blend(
                    blend( mip[ mip_level + 1 ][ u_index ], mip[ mip_level + 1 ][ u_index + 1 ],( vi.x >> ( 8 + mip_level + 1 ) ) & 0xff ),
                    blend( mip[ mip_level ][ u_index + mip_dim[ mip_level + 1 ].x ], mip[ mip_level ][ u_index + mip_dim[ mip_level + 1 ].x + 1 ],( vi.x >> ( 8 + mip_level + 1 ) ) & 0xff ),
                    ( vi.y >> ( 8 + mip_level + 1 ) ) & 0xff 
                ),
                mip_blend >> 8
            );
}

template<> void uimage::dump() {
    for( auto& v : base ) { std::cout << std::hex << v; }
}

