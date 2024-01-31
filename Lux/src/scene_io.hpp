#include "scene.hpp"
#include "json.hpp"
#include "next_element.hpp"
#include "any_function.hpp"
#include "life.hpp"

struct scene_reader {
    using json = nlohmann::json;

    scene& s;

    std::map< std::string, std::string > elem_img_bufs, elem_mask_bufs; // Handle forward references to element buffers
    std::map< std::string, std::string > CA_targets; // Handle forward references to cellular automata target buffers
    std::map< std::string, std::string > cluster_elements; // Element for each cluster. Elements copied to clusters after buffers added to elements.
    std::map< std::string, std::string > fill_warp_vfs; // Vector field for each fill_warp effect. Vector fields copied to fill_warps after buffers added to vector fields.

    scene_reader( scene& s_init, std::string( filename ) );

    void add_default_functions();

    void read( bool& b,   const json& j ) { b  = j.get<bool>();   }
    void read( int& i,    const json& j ) { i  = j.get<int>();    }
    void read( float& f,  const json& j ) { f  = j.get<float>();  }
    void read( vec2f& v,  const json& j ) { v  = read_vec2f( j ); }
    void read( vec2i& v,  const json& j ) { v  = read_vec2i( j ); }
    void read( interval_float& i, const json& j ) { i = read_interval_float( j ); }
    void read( interval_int& i,   const json& j ) { i = read_interval_int( j );   }
    void read( bb2f& bb,  const json& j ) { bb = read_bb2f( j );  }
    void read( bb2i& bb,  const json& j ) { bb = read_bb2i( j );  }
    void read( frgb& f,   const json& j ) { f  = read_frgb( j );  }
    void read( ucolor& u, const json& j ) { u  = read_ucolor( j );}
    void read( unsigned long long& ull, const json& j ) { ull = read_ull( j ); }
    void read( std::string& s,  const json& j ) { s = read_string( j ); }
    void read( rotation_direction& r, const json& j ) { r = read_rotation_direction( j ); }
    void read( direction4& d,   const json& j ) { d = read_direction4( j ); }
    void read( direction8& d,   const json& j ) { d = read_direction8( j ); }
    void read( pixel_type& p,   const json& j ) { p = read_pixel_type( j ); }
    void read( image_extend& e, const json& j ) { e = read_image_extend( j ); }
    void read( render_mode& r,  const json& j ) { r = read_render_mode( j ); }
    void read( CA_hood& h,      const json& j ) { h = read_hood( j ); }
    void read( menu_type& m,    const json& j ) { m = read_menu_type( j ); }
    void read( switch_type& s,  const json& j ) { s = read_switch_type( j ); }
    void read( std::optional< int   >& i, const json& j ) { i = j.get<int>();  }    
    void read( std::optional< float >& f, const json& j ) { f = j.get<float>();  }  
    void read( std::vector< vec2f >& v,   const json& j ) { for( auto& k : j ) { v.push_back( read_vec2f( k ) ); } }

    vec2f  read_vec2f(  const json& j );
    vec2i  read_vec2i(  const json& j );
    interval_float read_interval_float( const json& j );
    interval_int   read_interval_int(   const json& j );
    frgb   read_frgb(   const json& j );
    bb2f   read_bb2f(   const json& j );
    bb2i   read_bb2i(   const json& j );
    ucolor read_ucolor( const json& j );  // hexadecimal color
    unsigned long long read_ull(    const json& j ); // hexadecimal
    std::string  read_string(       const json& j );
    rotation_direction read_rotation_direction( const json& j );
    direction4   read_direction4(   const json& j );
    direction8   read_direction8(   const json& j );
    pixel_type   read_pixel_type(   const json& j );
    image_extend read_image_extend( const json& j );
    render_mode  read_render_mode(  const json& j );
    CA_hood      read_hood(         const json& j );
    menu_type    read_menu_type(    const json& j );
    switch_type  read_switch_type(  const json& j );

    void read_image(    const json& j );
    void read_rule(     const json& j, std::shared_ptr< CA_ucolor >& ca );   // Cellular automata rule
    void read_effect(   const json& j );
    void read_element(  const json& j );
    void read_function( const json& j );
    void read_widget_group( const json& j );
    void read_cluster(  const json& j );
    void read_queue(    const json& j, effect_list& elist );

    template< class T > void read_harness( const json& j, harness< T >& h, std::unordered_map< std::string, any_fn< T > >& harness_fns );
    #define READ_ANY_HARNESS( _T_, _U_ ) void read_any_harness( const json& j, harness< _T_ >& h )  { read_harness< _T_ >( j, h, _U_ ); }
    READ_ANY_HARNESS( float,  s.float_fns )
    READ_ANY_HARNESS( int,    s.int_fns )
    READ_ANY_HARNESS( vec2f,  s.vec2f_fns )
    READ_ANY_HARNESS( vec2i,  s.vec2i_fns )
    READ_ANY_HARNESS( bb2f,   s.bb2f_fns )
    READ_ANY_HARNESS( bb2i,   s.bb2i_fns )
    READ_ANY_HARNESS( frgb,   s.frgb_fns )
    READ_ANY_HARNESS( ucolor, s.ucolor_fns )
    READ_ANY_HARNESS( std::string, s.string_fns )
    READ_ANY_HARNESS( bool,   s.bool_fns )
    READ_ANY_HARNESS( direction4, s.direction4_fns )
    READ_ANY_HARNESS( direction8, s.direction8_fns )

    //READ_ANY_HARNESS( std::optional< int > )
    //READ_ANY_HARNESS( std::optional< float > )
    //READ_ANY_HARNESS( std::vector< vec2f > )
};