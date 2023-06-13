#include "scene.hpp"
#include "scene_io.hpp"
#include "life.hpp"
#include "fimage.hpp"
#include "uimage.hpp"
#include "vector_field.hpp"
#include <fstream>
#include <sstream>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
void emscripten_message( std::string msg ) {
    msg = "console.log('" + msg + "');";
    emscripten_run_script( msg.c_str() );
}

void emscripten_error( std::string msg ) {
    emscripten_message( msg );
    exit( 0 );
}

#define DEBUG( msg ) emscripten_message( msg );
#define ERROR( msg ) emscripten_error( msg ); 
#else
#define DEBUG( msg ) { std::string debug_msg = msg; std::cout << debug_msg << std::endl; }
#define ERROR( msg ) throw std::runtime_error( msg );
#endif

using json = nlohmann::json;

scene_reader::scene_reader( scene& s_init, std::string( filename ) ) : s( s_init ) {
    // read a JSON file
    DEBUG( "scene_reader constructor" );
#ifdef __EMSCRIPTEN__
    DEBUG( "emscripten defined" )
#endif // __EMSCRIPTEN__
    std::ifstream in_stream(filename);
    json j;
    DEBUG( "input stream opened" )
    try {
        j = json::parse(in_stream); 
    }
    catch( json::parse_error& ex ) {
        ERROR( "JSON parse error at byte " + std::to_string( ex.byte ) )
        exit( 0 );
    }
    DEBUG( "scene file parsed into json object" )

    // scene fields
    if( j.contains( "name" ) ) j[ "name" ].get_to( s.name ); else s.name = "Unnamed";
    DEBUG( "Name: " + s.name )
    //if( j.contains( "size" ) ) s.size = read_vec2i( j[ "size" ] ); else s.size = { 1080, 1080 };
    if( j.contains( "images" ) )    for( auto& jimg :   j[ "images" ] )    read_image( jimg );
    DEBUG( "Images loaded" ) 
    if( j.contains( "elements" ) )  for( auto& jelem :  j[ "elements"  ] ) read_element(  jelem  ); 
    DEBUG( "Elements loaded" ) 
    add_default_functions();
    if( j.contains( "functions" ) ) for( auto& jfunc :  j[ "functions" ] ) read_function( jfunc  );
    
    if( j.contains( "clusters" ) )  for( auto& jclust : j[ "clusters"  ] ) read_cluster(  jclust ); // if no clusters, blank scene
    // render list - list of images, elements, clusters, and effects (each represented as an effect)
    if( j.contains( "effects" ) )   for( auto& jeff : j[ "effects" ] )     read_effect( jeff );
    DEBUG( "Effects loaded" )

    if( j.contains( "queue" ) ) for( auto& q : j[ "queue" ] ) {
        effect_list eff_list;
        read_queue( q, eff_list );
        s.queue.push_back( eff_list );              // add to render queue
        s.buffers[ eff_list.name ] = eff_list.buf;  // add to buffer map
    }

    // set element buffers, copy elements to clusters
    for( auto& e : elem_img_bufs  ) { s.elements[ e.first ]->img  = s.buffers[ e.second ];  }
    for( auto& m : elem_mask_bufs ) { s.elements[ m.first ]->mask = s.buffers[ m.second ];  }
    for( auto& c : cluster_elements ) { s.clusters[ c.first ]->root_elem = *s.elements[ c.second ]; }
    DEBUG( "scene_reader constructor finished" )
}

vec2f scene_reader::read_vec2f( const json& j ) { return vec2f( { j[0], j[1] } ); }
vec2i scene_reader::read_vec2i( const json& j ) { return vec2i( { j[0], j[1] } ); }
frgb  scene_reader::read_frgb(  const json& j ) { return frgb( j[0], j[1], j[2] ); }
bb2f  scene_reader::read_bb2f(  const json& j ) { return bb2f( read_vec2f( j[0] ), read_vec2f( j[1]) ); }
bb2i  scene_reader::read_bb2i(  const json& j ) { return bb2i( read_vec2i( j[0] ), read_vec2i( j[1]) ); }
std::string scene_reader::read_string( const json& j ) { std::string s; j.get_to( s ); return s; }

// ucolor represented as hexidecimal string
ucolor scene_reader::read_ucolor( const json& j ) { 
    std::string color;
    ucolor u;

    j.get_to( color );
    std::istringstream( color ) >> std::hex >> u;
    return u;
}

direction4 scene_reader::read_direction4( const json& j ) { 
    std::string s;
    direction4 d;

    j.get_to( s );
    if(      s == "up"   ) d = direction4::D4_UP;
    else if( s == "right") d = direction4::D4_RIGHT;
    else if( s == "down" ) d = direction4::D4_DOWN;
    else if( s == "left" ) d = direction4::D4_LEFT;
    else ERROR( "Invalid direction4 string: " + s )
    return d;
}

pixel_type scene_reader::read_pixel_type( const json& j ) { 
    std::string s;
    pixel_type p;

    j.get_to( s );
    if(      s == "fimage"       || s == "frgb"   ) p = pixel_type::PIXEL_FRGB;
    else if( s == "uimage"       || s == "ucolor" ) p = pixel_type::PIXEL_UCOLOR;
    else if( s == "vector_field" || s == "vec2f"  ) p = pixel_type::PIXEL_VEC2F;
    else if( s == "offset_field" || s == "vec2i"  ) p = pixel_type::PIXEL_VEC2I;
    else if( s == "warp_field"   || s == "int"    ) p = pixel_type::PIXEL_INT;
    else ERROR( "Invalid pixel_type string: " + s )
    return p;
}

image_extend scene_reader::read_image_extend( const json& j ) { 
    std::string s;
    image_extend e;

    j.get_to( s );
    if(      s == "single"   ) e = image_extend::SAMP_SINGLE;
    else if( s == "repeat"   ) e = image_extend::SAMP_REPEAT;
    else if( s == "reflect"  ) e = image_extend::SAMP_REFLECT;
    else ERROR( "Invalid image_extend string: " + s )
    return e;
}

render_mode scene_reader::read_render_mode( const json& j ) { 
    std::string s;
    render_mode r;

    j.get_to( s );
    if(      s == "static"    ) r = render_mode::MODE_STATIC;
    else if( s == "iterative" ) r = render_mode::MODE_ITERATIVE;
    else if( s == "ephemeral" ) r = render_mode::MODE_EPHEMERAL;
    else ERROR( "Invalid render_mode string: " + s )
    return r;
}

void scene_reader::read_image( const json& j ) {
    std::string type, name, filename;
    DEBUG( "scene_reader::read_image" )

    if( j.contains( "filename" ) ) j[ "filename" ].get_to( filename );
    else ERROR( "scene_reader::read_image error - image filename missing\n" )
    DEBUG( "scene_reader::read_image - filename: " + filename )

    if( j.contains( "name" ) ) j[ "name" ].get_to( name );
    else name = filename;
    DEBUG( "scene_reader::read_image - name: " + name )

    if( j.contains( "type") ) j[ "type" ].get_to( type );
    else ERROR( "scene_reader::read_image error - image type missing\n" )
    DEBUG( "scene_reader::read_image - type: " + type )

    // future: add binary file format for all image types

    if( type == "fimage" ) {
        fbuf_ptr img( new buffer_pair< frgb >( filename ) );
        s.buffers[ name ] = img;
    }

    if( type == "uimage" ) {
        DEBUG( "scene_reader::read_image - reading uimage" )
        ubuf_ptr img( new buffer_pair< ucolor >( filename ) );
        s.buffers[ name ] = img;
        std :: cout << "pointer " << img << std::endl;
        //s.buffers[ name ] = std::make_shared< buffer_pair< ucolor > >( filename );
    }
    // future: binary image format, which will support fimage, uimage, and additionally vector_field
}

void scene_reader::read_element( const json& j ) {
    std::string name;
    vec2f position; 		    // coordinates of element center (relative to parent cluster)
    float scale; 			    // radius of element
    float rotation; 	        // image rotation in degrees
    float orientation;          // direction of motion relative to vector field ( or function )
    bool  orientation_lock;     // is rotation relative to orientation?
    int index;                  // index of element within cluster
    std::string mask_mode;      // how will mask be applied to splat and backround?

    std::string img;  // If no image, element not rendered, serves as placeholder
    std::string mask;

    if( j.contains( "name" ) ) j[ "name" ].get_to( name );  else ERROR( "Element name missing\n" )
    s.elements[ name ] = std::make_shared< element >();
    element& elem = *(s.elements[ name ]);


    if( j.contains( "position" ) ) elem.position = read_vec2f( j[ "position" ] );
    if( j.contains( "scale" ) )       j[ "scale"       ].get_to( elem.scale );
    if( j.contains( "rotation" ) )    j[ "rotation"    ].get_to( elem.rotation );
    if( j.contains( "orientation" ) ) j[ "orientation" ].get_to( elem.orientation );
    if( j.contains( "orientation_lock" ) ) j[ "orientation_lock" ].get_to( elem.orientation_lock );
    // Set initial derivative - may need to be modified by initializers in next_element
    if( j.contains( "derivative" ) ) elem.derivative = read_vec2f( j[ "derivative" ] );
    if( j.contains( "derivative_lock" ) ) j[ "derivative_lock" ].get_to( elem.derivative_lock );
    if( j.contains( "mask_mode" ) )   j[ "mask_mode"   ].get_to( mask_mode );
    
    if( mask_mode == "no_effect" ) elem.mmode = MASK_NOEFFECT;
    if( mask_mode == "trim"      ) elem.mmode = MASK_TRIM;
    if( mask_mode == "opacity"   ) elem.mmode = MASK_OPACITY;
    if( mask_mode == "blend"     ) elem.mmode = MASK_BLEND;

    // image, mask, tint
    if( j.contains( "image" ) ) {
        j[ "image" ].get_to( img );
        elem_img_bufs[ name ] = img;
    }

    if( j.contains( "mask" ) ) {
        j[ "mask" ].get_to( mask );
        elem_mask_bufs[ name ] = mask;
    }

    if( j.contains( "tint" ) ) {
        auto k = j[ "tint" ];
        if( k.contains( "frgb"   ) ) elem.tint = read_frgb(   k[ "frgb" ]   );
        if( k.contains( "ucolor" ) ) elem.tint = read_ucolor( k[ "ucolor" ] );
        if( k.contains( "vec2f"  ) ) elem.tint = read_vec2f(  k[ "vec2f" ]  );
    }
}

// forward references not implemented
template< class T > void scene_reader::read_harness( const json& j, harness< T >& h, std::unordered_map< std::string, any_fn< T > >& harness_fns ) {
    if( j.is_object() ) {
        if( j.contains( "value" ) ) { 
            read( h.val, j[ "value" ] );
        }
        // need list of harness functions by type (map of maps?)
        if( j.contains( "functions" ) ) for( std::string name : j[ "functions" ] ) { 
            h.add_function( harness_fns[ name ] );
        }
    }
    else read( h.val, j );
}

void scene_reader::add_default_functions() {
    // UI functions
    std::shared_ptr< mouse_pos_fn > mouse_position( new mouse_pos_fn );
    s.vec2f_fns[ "mouse_position" ] = any_fn< vec2f >( mouse_position, std::ref( *mouse_position ), "mouse_position" );
    std::shared_ptr< mouse_pix_fn > mouse_pixel( new mouse_pix_fn );
    s.vec2i_fns[ "mouse_pixel" ] = any_fn< vec2i >( mouse_pixel, std::ref( *mouse_pixel ), "mouse_pixel" );

    // UI conditions
    std::shared_ptr< mousedown_condition > mouse_down( new mousedown_condition );
    s.condition_fns[ "mouse_down" ] = any_condition_fn( mouse_down, std::ref( *mouse_down ), "mouse_down" );
    std::shared_ptr< mouseover_condition > mouse_over( new mouseover_condition );
    s.condition_fns[ "mouse_over" ] = any_condition_fn( mouse_over, std::ref( *mouse_over ), "mouse_over" );

    // Cluster conditions
    std::shared_ptr< initial_element_condition > initial_element( new initial_element_condition );
    s.condition_fns[ "initial_element" ] = any_condition_fn( initial_element, std::ref( *initial_element ), "initial_element" );
    std::shared_ptr< following_element_condition > following_element( new following_element_condition );
    s.condition_fns[ "following_element" ] = any_condition_fn( following_element, std::ref( *following_element ), "following_element" );
    std::shared_ptr< top_level_condition > top_level( new top_level_condition );
    s.condition_fns[ "top_level" ] = any_condition_fn( top_level, std::ref( *top_level ), "top_level" );
    std::shared_ptr< lower_level_condition > lower_level( new lower_level_condition );
    s.condition_fns[ "lower_level" ] = any_condition_fn( lower_level, std::ref( *lower_level ), "lower_level" );

}

void scene_reader::read_function( const json& j ) {
    std::string name, type, fn_name;

    if( j.contains( "name" ) ) j[ "name" ].get_to( name );  else ERROR( "Function name missing\n" )
    if( j.contains( "type" ) ) j[ "type" ].get_to( type );  else ERROR( "Function type missing\n" );

    // special case for conditionals
    if( type == "filter" ) {
        std::shared_ptr< filter > fn ( new filter );
        any_gen_fn gen_func( fn, std::ref( *fn ), name );
        if( j.contains( "conditions" ) ) for( auto& c : j[ "conditions" ] ) fn->add_condition( s.condition_fns[ c ] );
        if( j.contains( "functions"  ) ) for( auto& f : j[ "functions"  ] ) fn->add_function(  s.gen_fns[  f ] );
        s.gen_fns[ name ] = gen_func;
    }

    // example of expanded macro sequence
    /* if( type == "orientation_gen_fn" ) { 
        auto fn = orientation_gen_fn(); 
        any_gen_fn func( std::make_shared< orientation_gen_fn >( fn ), std::ref( fn ), name );
        if( j.contains( "orientation" ) ) read_any_harness( j[ "orientation" ], fn. orientation );
        s.functions[ name ] = func;
    } */

    #define FN( _T_, _U_ ) if( type == #_T_ ) {  std::shared_ptr< _T_ > fn( new _T_ ); any_fn< _U_ >    func( fn, std::ref( *fn ), name );
    #define END_FN( _T_ )  s. _T_##_fns[ name ] = func; }
    #define GEN_FN( _T_ )  if( type == #_T_ ) {  std::shared_ptr< _T_ > fn( new _T_ ); any_gen_fn       func( fn, std::ref( *fn ), name ); 
    #define END_GEN_FN()   s.gen_fns[ name ] = func; }
    #define COND_FN( _T_ ) if( type == #_T_ ) {  std::shared_ptr< _T_ > fn( new _T_ ); any_condition_fn func( fn, std::ref( *fn ), name );
    #define END_COND_FN()  s.condition_fns[ name ] = func; }
    #define HARNESS( _T_ ) if( j.contains( #_T_ ) ) read_any_harness( j[ #_T_ ], fn-> _T_ );
    #define READ( _T_ )    if( j.contains( #_T_ ) ) read( fn-> _T_, j[ #_T_ ] );
    #define PARAM( _T_ )   if( j.contains( "fn" ) ) { j[ "fn" ].get_to( fn_name ); fn->fn = s. _T_##_fns[ fn_name ]; }

    // harness float functions
    FN( adder_float, float ) HARNESS( r ) END_FN( float )
    FN( log_fn, float      ) HARNESS( scale ) HARNESS( shift ) END_FN( float )
    FN( ratio_float, float ) HARNESS( r ) END_FN( float )
    FN( wiggle, float      ) HARNESS( wavelength ) HARNESS( amplitude ) HARNESS( phase ) HARNESS( wiggliness ) END_FN( float )

    // harness vec2f functions
    FN( adder_vec2f, vec2f  ) HARNESS( r ) END_FN( vec2f )
    FN( ratio_vec2f, vec2f  ) HARNESS( r ) END_FN( vec2f )
    FN( mouse_pos_fn, vec2f ) END_FN( vec2f )

    // parameter functions
    FN( index_param_float, float ) PARAM( float ) END_FN( float )
    FN( scale_param_float, float ) PARAM( float ) END_FN( float )
    FN( time_param_float,  float ) PARAM( float ) END_FN( float )

    // single field modifiers
    GEN_FN( orientation_gen_fn ) HARNESS( orientation ) END_FN( gen )
    GEN_FN( scale_gen_fn       ) HARNESS( scale ) END_FN( gen )
    GEN_FN( position_gen_fn    ) HARNESS( position ) END_FN( gen )

    // generalized functions (alphabetical order)
    GEN_FN( advect_element ) HARNESS( flow ) HARNESS( step ) READ( proportional ) READ( orientation_sensitive ) END_FN( gen )
    GEN_FN( angle_branch ) READ( interval ) READ( offset ) READ( mirror_offset ) HARNESS( size_prop ) HARNESS( branch_ang ) HARNESS( branch_dist ) END_FN( gen )
    GEN_FN( curly ) HARNESS( curliness ) END_FN( gen )
    // position_list should go here - figure out how to work the vector of positions

    // condition functions
    COND_FN( switch_condition ) READ( val ) END_COND_FN()
    COND_FN( random_condition ) HARNESS( p ) END_COND_FN()
    COND_FN( random_sticky_condition ) HARNESS( p_start ) HARNESS( p_change_true ) HARNESS( p_change_false ) END_COND_FN()
}

void scene_reader::read_cluster( const json& j ) {
    std::string name;
    element root_elem;  // initial element in cluster
    std::string root_elem_name;
    int max_depth;      // prevent infinite recursion
    float min_scale;    // approximately one pixel
    bb2f bounds;        // Optionally, cluster will stop generating if it goes out of bounds
    bool tlc;

    // Required fields
    if( j.contains( "name" ) )          j[ "name" ].get_to( name );  else ERROR( "Cluster name missing\n" )
    // Check for unique name. Future - make sure duplicate clusters refer to the same cluster
    if( s.clusters.contains( name ) )   ERROR( "Cluster name collision\n" )
    if( j.contains( "element" ) )     j[ "element" ].get_to( root_elem_name ); else ERROR( "Cluster root_elem missing\n" )

    // create cluster object
    s.next_elements[ name ] = std::make_shared< next_element >();
    s.clusters[ name ] = std::make_shared< cluster >( *s.elements[ root_elem_name ], *s.next_elements[ name ] );
    cluster& clust = *(s.clusters[ name ]);   // reference to cluster object
    cluster_elements[ name ] = root_elem_name;

    // optional fields
    if( j.contains( "max_depth" ) )  read_any_harness( j[ "max_depth" ], clust.max_depth );
    if( j.contains( "min_scale" ) )  read_any_harness( j[ "min_scale" ], clust.min_scale );
    if( j.contains( "max_n" ) )      read_any_harness( j[ "max_n"     ], clust.max_n );
    if( j.contains( "functions" ) )  for( std::string fname : j[ "functions"  ] ) clust.next_elem.add_function( s.gen_fns[ fname ] ); 
    if( j.contains( "conditions" ) ) for( std::string fname : j[ "conditions" ] ) clust.next_elem.add_condition( s.condition_fns[ fname ] ); 
}

void scene_reader::read_rule( const json& j, std::shared_ptr< CA_ucolor >& ca ) {
    std::string name, type;

    // Required fields
    if( j.contains( "name" ) )          j[ "name" ].get_to( name );  else ERROR( "CA rule name missing\n" )
    if( j.contains( "type" ) )          j[ "type" ].get_to( type );  else ERROR( "CA rule type missing\n" )

    #define RULE( _T_ )    if( type == #_T_ ) {  std::shared_ptr< _T_ > r( new _T_ ); any_rule rule( r, std::ref( *r ), r->neighborhood, name ); ca->rule = rule;
    #define HARNESSR( _T_ ) if( j.contains( #_T_ ) ) read_any_harness( j[ #_T_ ], r-> _T_ );
    #define READR( _T_ )    if( j.contains( #_T_ ) ) read( r-> _T_, j[ #_T_ ] );
    #define END_RULE()     s.CA_rules[ name ] = rule; }

    RULE( rule_life_ucolor ) END_RULE()
    RULE( rule_diffuse_ucolor)     READR( alpha_block ) END_RULE()
    RULE( rule_gravitate_ucolor )  READR( direction ) READR( alpha_block ) END_RULE()
    RULE( rule_snow_ucolor )       READR( direction ) READR( alpha_block ) END_RULE()
    RULE( rule_pixel_sort_ucolor ) READR( direction ) READR( alpha_block ) HARNESSR( max_diff ) END_RULE()
}

void scene_reader::read_effect( const json& j ) {
    std::string name, type, buf_name;

    // Required fields
    if( j.contains( "name" ) )          j[ "name" ].get_to( name );  else ERROR( "Effect name missing\n" )
    if( j.contains( "type" ) )          j[ "type" ].get_to( type );  else ERROR( "Effect type missing\n" )
    // Check for unique name. Future - make sure duplicate effects refer to the same effect

    if( s.effects.contains( name ) )   ERROR( "Effect name collision\n" )

    #define EFF( _T_ )     if( type == #_T_ ) {  std::shared_ptr< _T_ > e( new _T_ ); any_effect_fn eff( e, std::ref( *( e.get() ) ), name );
    #define HARNESSE( _T_ ) if( j.contains( #_T_ ) ) read_any_harness( j[ #_T_ ], e-> _T_ );
    #define READE( _T_ )    if( j.contains( #_T_ ) ) read( e-> _T_, j[ #_T_ ] );
    #define END_EFF()      s.effects[ name ] = eff; }

    // special case for element and cluster effects
    if( type == "element" ) {
        if( j.contains( "element_name" ) ) {
            std::string elem_name;
            j[ "element_name" ].get_to( elem_name );
            if( s.elements.contains( elem_name ) ) {
                //std::shared_ptr< element > e( new element( *s.elements[ elem_name ] ) );
                s.effects[ name ] = any_effect_fn( s.elements[ elem_name ], std::ref( *( s.elements[ elem_name ].get() ) ), name );
            }
            else ERROR( "element effect not found\n" )
        }
        else ERROR( "element effect missing\n" )
    }

    if( type == "cluster" ) {
        if( j.contains( "cluster_name" ) ) {
            std::string clust_name;
            j[ "cluster_name" ].get_to( clust_name );
            if( s.clusters.contains( clust_name ) ) {
                //std::shared_ptr< cluster > c( new cluster( *s.clusters[ clust_name ] ) );
                s.effects[ name ] = any_effect_fn( s.clusters[ clust_name ], std::ref( *( s.clusters[ clust_name ].get() ) ), name );
            }
            else ERROR( "cluster effect not found\n" )
        }
        else ERROR( "cluster effect missing\n" )
    }

    // special case for CA rules
    EFF( CA_ucolor )
    if( j.contains( "rule" ) ) read_rule( j[ "rule" ], e );
    else ERROR( "CA rule missing\n" )
    END_EFF()

    // special case for effects running effects
    EFF( eff_n ) HARNESSE( n ) 
    if( j.contains( "eff" ) ) {
        std::string eff_name;
        j[ "eff" ].get_to( eff_name );
        if( s.effects.contains( eff_name ) ) e->eff = s.effects[ eff_name ];
        else ERROR( "eff_n effect not found\n" )
    }
    // else identity effect - should be automatic
    END_EFF()

    EFF( eff_composite )
    if( j.contains( "effects ") )
    {
        for( std::string eff_name : j[ "effects" ] ) {
            if( s.effects.contains( eff_name ) ) e->add_effect( s.effects[ eff_name ] );
            else ERROR( "eff_composite effect not found\n" )
        }
    }
    // else empty effect list - should be automatic
    END_EFF()

    EFF( eff_fill_frgb )   HARNESSE( fill_color ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_fill_ucolor ) HARNESSE( fill_color ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_fill_vec2i )  HARNESSE( fill_color ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_fill_vec2f )  HARNESSE( fill_color ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_fill_int )    HARNESSE( fill_color ) READE( bounded ) HARNESSE( bounds ) END_EFF()

    EFF( eff_noise_frgb )   HARNESSE( a ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_noise_ucolor ) HARNESSE( a ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_noise_vec2i )  HARNESSE( a ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_noise_vec2f )  HARNESSE( a ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_noise_int )    HARNESSE( a ) READE( bounded ) HARNESSE( bounds ) END_EFF()

    EFF( eff_vector_warp_frgb )   READE( vf_name ) HARNESSE( step ) READE( smooth ) READE( relative ) READE( extend ) END_EFF()
    EFF( eff_vector_warp_ucolor ) READE( vf_name ) HARNESSE( step ) READE( smooth ) READE( relative ) READE( extend ) END_EFF()
    EFF( eff_vector_warp_vec2i )  READE( vf_name ) HARNESSE( step ) READE( smooth ) READE( relative ) READE( extend ) END_EFF()
    EFF( eff_vector_warp_vec2f )  READE( vf_name ) HARNESSE( step ) READE( smooth ) READE( relative ) READE( extend ) END_EFF()
    EFF( eff_vector_warp_int )    READE( vf_name ) HARNESSE( step ) READE( smooth ) READE( relative ) READE( extend ) END_EFF()

    EFF( eff_feedback_frgb )   READE( wf_name ) END_EFF()
    EFF( eff_feedback_ucolor ) READE( wf_name ) END_EFF()
    EFF( eff_feedback_vec2i )  READE( wf_name ) END_EFF()
    EFF( eff_feedback_vec2f )  READE( wf_name ) END_EFF()
    EFF( eff_feedback_int )    READE( wf_name ) END_EFF()
}

void scene_reader::read_queue( const json& j, effect_list& elist ) {
    if( j.contains( "name"         ) ) read( elist.name,           j[ "name"         ] );
    if( j.contains( "source"       ) ) read( elist.source_name,    j[ "source"       ] );
    if( j.contains( "effects"      ) ) for( std::string eff_name : j[ "effects"      ] ) elist.effects.push_back( eff_name );  
    if( j.contains( "relative_dim" ) ) read( elist.relative_dim,   j[ "relative_dim" ] );  
    if( j.contains( "mode"         ) ) read( elist.rmode,          j[ "mode"         ] );
    if( j.contains( "type"         ) ) read( elist.ptype,          j[ "type"         ] );
}
