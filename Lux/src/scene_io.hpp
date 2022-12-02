#include "scene.hpp"
#include "json.hpp"
#include "next_element.hpp"

struct scene_reader {
    using json = nlohmann::json;

    scene& s;

    scene_reader( scene& s_init, std::string( filename ) );

    void read( bool& b,   const json& j ) { b =  j.get<float>();  }
    void read( int& i,    const json& j ) { i =  j.get<float>();  }
    void read( float& f,  const json& j ) { f =  j.get<float>();  }
    void read( vec2f& v,  const json& j ) { v =  read_vec2f( j ); }
    void read( vec2i& v,  const json& j ) { v =  read_vec2i( j ); }
    void read( bb2f& bb,  const json& j ) { bb = read_bb2f( j );  }
    void read( bb2i& bb,  const json& j ) { bb = read_bb2i( j );  }
    void read( frgb& f,   const json& j ) { f =  read_frgb( j );  }
    void read( ucolor& u, const json& j ) { u =  read_ucolor( j );}
    void read( std::optional< int   >& i, const json& j ) { i = j.get<int>();  }    
    void read( std::optional< float >& f, const json& j ) { f = j.get<float>();  }  
    void read( std::vector< vec2f >& v,   const json& j ) { for( auto k : j ) { v.push_back( read_vec2f( k ) ); } }

    vec2f  read_vec2f(  const json& j );
    vec2i  read_vec2i(  const json& j );
    frgb   read_frgb(   const json& j );
    bb2f   read_bb2f(   const json& j );
    bb2i   read_bb2i(   const json& j );
    ucolor read_ucolor( const json& j );

    void  read_image(    const json& j );
    void  read_effect(   const json& j );
    void  read_element(  const json& j );
    void  read_function( const json& j );
    void  read_cluster(  const json& j );

    template< class T > void read_harness( const json& j, harness< T >& h, std::map< std::string, std::function< T ( T&, element_context& ) > >& harness_fns );
    #define READ_ANY_HARNESS( _T_, _U_ ) void read_any_harness( const json& j, harness< _T_ >& h )  { read_harness< _T_ >( j, h, _U_ ); }
    READ_ANY_HARNESS( float, s.float_fns )
    READ_ANY_HARNESS( int, s.int_fns )
    READ_ANY_HARNESS( vec2f, s.vec2f_fns )
    READ_ANY_HARNESS( vec2i, s.vec2i_fns )
    //READ_ANY_HARNESS( std::optional< int > )
    //READ_ANY_HARNESS( std::optional< float > )
    //READ_ANY_HARNESS( std::vector< vec2f > )
};