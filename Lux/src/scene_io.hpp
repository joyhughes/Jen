#ifndef SCENE_IO_HPP
#define SCENE_IO_HPP

#include "scene.hpp"
#include "json.hpp"
#include "next_element.hpp"
#include "any_function.hpp"
#include "life.hpp"
#include <sstream>

struct scene_reader {
    using json = nlohmann::json;

    scene& s;

    std::map< std::string, std::string > elem_img_bufs, elem_mask_bufs; // Handle forward references to element buffers
    std::map< std::string, std::string > CA_targets; // Handle forward references to cellular automata target buffers
    std::map< std::string, std::string > cluster_elements; // Element for each cluster. Elements copied to clusters after buffers added to elements.

    scene_reader( scene& s_init, std::string( filename ) );
    scene_reader( scene& s_init, const json& scene_json, bool load_runtime_state = false );

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
    void read( funk_factor& ull, const json& j ) { ull = read_funk_factor( j ); }
    void read( std::string& s,  const json& j ) { s = read_string( j ); }
    void read( rotation_direction& r, const json& j ) { r = read_rotation_direction( j ); }
    void read( direction4& d,   const json& j ) { d = read_direction4( j ); }
    void read( direction4_diagonal& d, const json& j ) { d = read_direction4_diagonal( j ); }
    void read( direction8& d,   const json& j ) { d = read_direction8( j ); }
    void read( box_blur_type& b, const json& j ) { b = read_box_blur_type( j ); }
    void read( pixel_type& p,   const json& j ) { p = read_pixel_type( j ); }
    void read( image_extend& e, const json& j ) { e = read_image_extend( j ); }
    void read( render_mode& r,  const json& j ) { r = read_render_mode( j ); }
    void read( CA_hood& h,      const json& j ) { h = read_hood( j ); }
    void read( probability_distribution& p, const json& j ) { p = read_probability_distribution( j ); }
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
    unsigned long long read_funk_factor(    const json& j ); // hexadecimal
    std::string  read_string(       const json& j );
    rotation_direction read_rotation_direction( const json& j );
    direction4   read_direction4(   const json& j );
    direction4_diagonal read_direction4_diagonal( const json& j );
    direction8   read_direction8(   const json& j );
    box_blur_type read_box_blur_type( const json& j );
    pixel_type   read_pixel_type(   const json& j );
    image_extend read_image_extend( const json& j );
    render_mode  read_render_mode(  const json& j );
    CA_hood      read_hood(         const json& j );
    probability_distribution read_probability_distribution( const json& j );
    menu_type    read_menu_type(    const json& j );
    switch_type  read_switch_type(  const json& j );

    void read_image(    const json& j );
    void read_rule(     const json& j, std::shared_ptr< CA_ucolor >& ca );   // Cellular automata rule
    void read_effect(   const json& j );
    void read_element(  const json& j );
    void read_function( const json& j );
    void read_widget_group( const json& j );
    void read_cluster(  const json& j );
    void read_queue(    const json& j );

    void initialize_from_json(const json& j, bool load_runtime_state);

    template< class T > void read_harness( const json& j, harness< T >& h );
    #define READ_ANY_HARNESS( _T_ ) void read_any_harness( const json& j, harness< _T_ >& h )  { read_harness< _T_ >( j, h ); }
    READ_ANY_HARNESS( float )
    READ_ANY_HARNESS( int )
    READ_ANY_HARNESS( funk_factor )
    READ_ANY_HARNESS( vec2f )
    READ_ANY_HARNESS( vec2i )
    READ_ANY_HARNESS( bb2f )
    READ_ANY_HARNESS( bb2i )
    READ_ANY_HARNESS( frgb )
    READ_ANY_HARNESS( ucolor )
    READ_ANY_HARNESS( std::string )
    READ_ANY_HARNESS( bool )
    READ_ANY_HARNESS( direction4 )
    READ_ANY_HARNESS( direction4_diagonal )
    READ_ANY_HARNESS( direction8 )
    READ_ANY_HARNESS( interval_float )
    READ_ANY_HARNESS( interval_int )
    READ_ANY_HARNESS( box_blur_type )
    READ_ANY_HARNESS( image_extend )

    //READ_ANY_HARNESS( std::optional< int > )
    //READ_ANY_HARNESS( std::optional< float > )
    //READ_ANY_HARNESS( std::vector< vec2f > )
    
    bool has_runtime_state(const json& scene_json);

private:
    bool is_saved_scene = false;
};

template<> struct any_fn< bool >;

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void to_json( nlohmann::json& j, const interval_float& i );
void to_json( nlohmann::json& j, const interval_int& i );
void to_json(nlohmann::json& j, const direction4& d );
void to_json(nlohmann::json& j, const direction4_diagonal& d );
void to_json(nlohmann::json& j, const direction8& d );
void to_json( nlohmann::json& j, const switch_fn& s );
void to_json( nlohmann::json& j, const any_function& af );
void to_json( nlohmann::json& j, const widget_group& wg );

struct scene_writer{
    using json = nlohmann::json;

    const scene& s;

    scene_writer( const scene& s_init );
    json write_scene_json() const;

    json write_images_json() const;
    json write_functions_json() const;
    json write_effects_json() const;
    json write_widget_groups_json() const;
    json write_queue_json() const;
private:
    json serialize_any_function(const any_function& f) const;
};

#endif // SCENE_IO_HPP