#include "scene.hpp"
#include "scene_io.hpp"
#include "life.hpp"
#include "fimage.hpp"
#include "uimage.hpp"
#include "vector_field.hpp"
#include "UI.hpp"
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
using string = std::string;

scene_reader::scene_reader( scene& s_init, std::string( filename ) ) : s( s_init ) {
    // read a JSON file
    DEBUG( "scene_reader constructor" );
#ifdef __EMSCRIPTEN__
    DEBUG( "emscripten defined" )
#endif // __EMSCRIPTEN__
    std::ifstream in_stream(filename);
    json j;
    DEBUG( "input stream opened " + filename )
    try {
        j = json::parse(in_stream); 
    }
    catch( json::parse_error& ex ) {
        DEBUG( "JSON parse error at byte " + std::to_string( ex.byte ) )
        ERROR( "JSON parse error at byte " + std::to_string( ex.byte ) )
        exit( 0 );
    }
    DEBUG( "scene file parsed into json object" )

    // scene fields
    if( j.contains( "name" ) ) j[ "name" ].get_to( s.name ); else s.name = "Unnamed";
    DEBUG( "Name: " + s.name )
    if( j.contains( "time_interval" ) ) j[ "time_interval" ].get_to( s.time_interval );

    // TODO: Allow size different from image size
    //if( j.contains( "size" ) ) s.size = read_vec2i( j[ "size" ] ); else s.size = { 1080, 1080 };
    if( j.contains( "images" ) )    for( auto& jimg :   j[ "images" ] )    read_image( jimg );
    DEBUG( "Images loaded" ) 
    if( j.contains( "elements" ) )  for( auto& jelem :  j[ "elements"  ] ) read_element(  jelem  ); 
    DEBUG( "Elements loaded" ) 
    add_default_functions();
    DEBUG( "Default functions added" )
    if( j.contains( "functions" ) ) for( auto& jfunc :  j[ "functions" ] ) read_function( jfunc  );
    DEBUG( "Functions loaded" )
    
    if( j.contains( "clusters" ) )  for( auto& jclust : j[ "clusters"  ] ) read_cluster(  jclust ); // if no clusters, blank scene
    DEBUG( "Clusters loaded" )
    // render list - list of images, elements, clusters, and effects (each represented as an effect)
    if( j.contains( "effects" ) )   for( auto& jeff : j[ "effects" ] )     read_effect( jeff );
    DEBUG( "Effects loaded" )

    if( j.contains( "queue" ) ) for( auto& q : j[ "queue" ] ) {
        read_queue( q );              // add to render queue
    }

    if( j.contains( "widget groups" ) ) for( auto& wg : j[ "widget groups" ] ) read_widget_group( wg );
    DEBUG( "Widget groups loaded" )

    // set element buffers, copy elements to clusters
    for( auto& e : elem_img_bufs  ) { s.elements[ e.first ]->img  = s.buffers[ e.second ];  }
    for( auto& m : elem_mask_bufs ) { s.elements[ m.first ]->mask = s.buffers[ m.second ];  }
    for( auto& c : cluster_elements ) { s.clusters[ c.first ]->root_elem = *s.elements[ c.second ]; }
    for( auto& t : CA_targets ) { 
        std::cout << "adding target buffer " << t.second << " to CA " << t.first << std::endl;
        std::get< std::shared_ptr< CA< ucolor > > >( s.effects[ t.first ].fn_ptr )->target = s.buffers[ t.second ]; 
    }
    DEBUG( "scene_reader constructor finished" )
}

vec2f scene_reader::read_vec2f( const json& j ) { return vec2f( { j[0], j[1] } ); }
vec2i scene_reader::read_vec2i( const json& j ) { return vec2i( { j[0], j[1] } ); }
interval_float scene_reader::read_interval_float( const json& j ) { return interval_float( j[0], j[1] ); }
interval_int   scene_reader::read_interval_int(   const json& j ) { return interval_int(   j[0], j[1] ); }
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

unsigned long long scene_reader::read_ull( const json& j ) { 
    std::string s;
    unsigned long long ull;
    j.get_to( s );
    std::istringstream( s ) >> std::hex >> ull;
    return ull;
}

rotation_direction scene_reader::read_rotation_direction( const json& j ) { 
    std::string s;
    rotation_direction r;

    j.get_to( s );
    if(      s == "counterclockwise" ) r = COUNTERCLOCKWISE;
    else if( s == "clockwise"        ) r = CLOCKWISE;
    else if( s == "random"           ) r = RANDOM;
    else if( s == "lava lamp"        ) r = LAVA_LAMP;
    else ERROR( "Invalid rotation_direction string: " + s )
    return r;
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

direction8 scene_reader::read_direction8( const json& j ) { 
    std::string s;
    direction8 d;

    j.get_to( s );
    if(      s == "up"         ) d = direction8::D8_UP;
    else if( s == "up_right"   ) d = direction8::D8_UPRIGHT;
    else if( s == "right"      ) d = direction8::D8_RIGHT;
    else if( s == "down_right" ) d = direction8::D8_DOWNRIGHT;
    else if( s == "down"       ) d = direction8::D8_DOWN;
    else if( s == "down_left"  ) d = direction8::D8_DOWNLEFT;
    else if( s == "left"       ) d = direction8::D8_LEFT;
    else if( s == "up_left"    ) d = direction8::D8_UPLEFT;
    else ERROR( "Invalid direction8 string: " + s )
    return d;
}

box_blur_type scene_reader::read_box_blur_type( const json& j ) { 
    std::string s;
    box_blur_type b;

    j.get_to( s );
    if(      s == "orthogonal" ) b = box_blur_type::BB_ORTHOGONAL;
    else if( s == "diagonal" )   b = box_blur_type::BB_DIAGONAL;
    else if( s == "all" )        b = box_blur_type::BB_ALL;
    else if( s == "custom" )     b = box_blur_type::BB_CUSTOM;
    else ERROR( "Invalid box_blur_type string: " + s )
    return b;
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

CA_hood scene_reader::read_hood( const json& j ) { 
    std::string s;
    CA_hood h;

    j.get_to( s );
    if(      s == "Moore" )             h = HOOD_MOORE;
    else if( s == "Margolus" )          h = HOOD_MARGOLUS;
    else if( s == "hourglass" )         h = HOOD_HOUR;
    else if( s == "hourglass_reverse" ) h = HOOD_HOUR_REV;
    else if( s == "bowtie" )            h = HOOD_BOW;
    else if( s == "bowtie_reverse" )    h = HOOD_BOW_REV;
    else if( s == "square" )            h = HOOD_SQUARE;
    else if( s == "square_reverse" )    h = HOOD_SQUARE_REV;
    else if( s == "random" )            h = HOOD_RANDOM;

    else ERROR( "Invalid CA_hood string: " + s )
    return h;
}

menu_type scene_reader::read_menu_type( const json& j ) { 
    std::string s;
    menu_type t;

    j.get_to( s );
    if(      s == "pull_down" )     t = MENU_PULL_DOWN;
    else if( s == "radio"    )      t = MENU_RADIO;
    else if( s == "image" )         t = MENU_IMAGE;
    else ERROR( "Invalid menu_type string: " + s )
    return t;
}

switch_type scene_reader::read_switch_type( const json& j ) { 
    std::string s;
    switch_type t;

    j.get_to( s );
    if(      s == "switch" ) t = SWITCH_SWITCH;
    else if( s == "checkbox" ) t = SWITCH_CHECKBOX;
    else ERROR( "Invalid switch_type string: " + s )
    return t;
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
        //std :: cout << "uimage pointer " << img << std::endl;
        //s.buffers[ name ] = std::make_shared< buffer_pair< ucolor > >( filename );
    }
    // future: binary image format, which will support fimage, uimage, and additionally vector_field
    DEBUG( "scene_reader::read_image - finished" )
}

void scene_reader::read_element( const json& j ) {
    std::string name, img, mask, mask_mode;
    // If no image, element not rendered, serves as placeholder

    if( j.contains( "name" ) ) j[ "name" ].get_to( name );  else ERROR( "Element name missing\n" )
    s.elements[ name ] = std::make_shared< element >();
    element& elem = *(s.elements[ name ]);

    if( j.contains( "smooth" ) ) j[ "smooth" ].get_to( elem.smooth );
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
    std::cout << "read element " << name << " smooth " << elem.smooth << std::endl;
}

// forward references not implemented
template< class T > void scene_reader::read_harness( const json& j, harness< T >& h ) {
    if( j.is_object() ) {
        if( j.contains( "value" ) ) { 
            read( h.val, j[ "value" ] );
        }
        // need list of harness functions by type (map of maps?)
        if( j.contains( "functions" ) ) for( std::string name : j[ "functions" ] ) { 
            h.add_function( std::get< any_fn< T > >( s.functions[ name ] ) );
        }
    }
    else read( h.val, j );
}

void scene_reader::add_default_functions() {
    // time function
    std::shared_ptr< time_fn > timer( new time_fn );
    s.functions[ "timer" ] = any_fn< float >( timer, std::ref( *timer ), "timer" );

    // UI functions
    std::shared_ptr< mouse_pos_fn > mouse_position( new mouse_pos_fn );
    s.functions[ "mouse_position" ] = any_fn< vec2f >( mouse_position, std::ref( *mouse_position ), "mouse_position" );
    std::shared_ptr< mouse_pix_fn > mouse_pixel( new mouse_pix_fn );
    s.functions[ "mouse_pixel" ] = any_fn< vec2i >( mouse_pixel, std::ref( *mouse_pixel ), "mouse_pixel" );

    // UI conditions
    std::shared_ptr< mousedown_condition > mouse_down( new mousedown_condition );
    s.functions[ "mouse_down" ] = any_condition_fn( mouse_down, std::ref( *mouse_down ), "mouse_down" );
    std::shared_ptr< mouseover_condition > mouse_over( new mouseover_condition );
    s.functions[ "mouse_over" ] = any_condition_fn( mouse_over, std::ref( *mouse_over ), "mouse_over" );
    std::shared_ptr< mouseclick_condition > mouse_click( new mouseclick_condition );
    s.functions[ "mouse_click" ] = any_condition_fn( mouse_click, std::ref( *mouse_click ), "mouse_click" );

    // Cluster conditions
    std::shared_ptr< initial_element_condition > initial_element( new initial_element_condition );
    s.functions[ "initial_element" ] = any_condition_fn( initial_element, std::ref( *initial_element ), "initial_element" );
    std::shared_ptr< following_element_condition > following_element( new following_element_condition );
    s.functions[ "following_element" ] = any_condition_fn( following_element, std::ref( *following_element ), "following_element" );
    std::shared_ptr< top_level_condition > top_level( new top_level_condition );
    s.functions[ "top_level" ] = any_condition_fn( top_level, std::ref( *top_level ), "top_level" );
    std::shared_ptr< lower_level_condition > lower_level( new lower_level_condition );
    s.functions[ "lower_level" ] = any_condition_fn( lower_level, std::ref( *lower_level ), "lower_level" );
}

void scene_reader::read_function( const json& j ) {
    std::string name, type, fn_name;

    if( j.contains( "name" ) ) j[ "name" ].get_to( name );  else ERROR( "Function name missing\n" )
    if( j.contains( "type" ) ) j[ "type" ].get_to( type );  else ERROR( "Function type missing\n" );

    DEBUG( "scene_reader::read_function - name: " + name + " type: " + type )

    // special case for conditionals
    if( type == "filter" ) {
        std::shared_ptr< filter > fn ( new filter );
        any_gen_fn gen_func( fn, std::ref( *fn ), name );
        if( j.contains( "conditions" ) ) for( auto& c : j[ "conditions" ] ) fn->add_condition( std::get< any_condition_fn >( s.functions[ c ] ) );
        if( j.contains( "functions"  ) ) for( auto& f : j[ "functions"  ] ) fn->add_function(  std::get< any_gen_fn       >( s.functions[ f ] ) );
        s.functions[ name ] = gen_func;
    }

    // example of expanded macro sequence
    /* if( type == "orientation_gen_fn" ) { 
        auto fn = orientation_gen_fn(); 
        any_gen_fn func( std::make_shared< orientation_gen_fn >( fn ), std::ref( fn ), name );
        if( j.contains( "orientation" ) ) read_any_harness( j[ "orientation" ], fn. orientation );
        s.functions[ name ] = func;
    } */

    #define FN( _T_, _U_ ) if( type == #_T_ )     { std::shared_ptr< _T_ > fn( new _T_ ); any_fn< _U_ >    func( fn, std::ref( *fn ), name );
    #define END_FN  s.functions[ name ] = func; }
    #define GEN_FN( _T_ )  if( type == #_T_ )     { std::shared_ptr< _T_ > fn( new _T_ ); any_gen_fn       func( fn, std::ref( *fn ), name ); 
    #define END_GEN_FN  s.functions[ name ] = func; }
    #define COND_FN( _T_ ) if( type == #_T_ )     { std::shared_ptr< _T_ > fn( new _T_ ); any_condition_fn func( fn, std::ref( *fn ), name );
    #define END_COND_FN  s.functions[ name ] = func; }

    #define HARNESS( _T_ ) if( j.contains( #_T_ ) ) read_any_harness( j[ #_T_ ], fn-> _T_ );
    #define READ( _T_ )    if( j.contains( #_T_ ) ) read( fn-> _T_, j[ #_T_ ] );
    #define PARAM( _T_ )   if( j.contains( "fn" ) ) { j[ "fn" ].get_to( fn_name ); fn->fn = std::get< any_fn< _T_ > >( s.functions[ fn_name ] ); }

    // harness bool functions
    FN( switch_fn, bool ) READ( tool ) READ( label ) READ( description ) READ( default_value ) fn->value = fn->default_value; END_FN

    // harness float functions
    //FN( adder_float, float ) HARNESS( r ) END_FN
    FN( log_fn,      float ) HARNESS( scale ) HARNESS( shift ) END_FN
    FN( time_fn,     float ) END_FN
    FN( ratio_float, float ) HARNESS( r ) END_FN
    FN( integrator_float, float ) HARNESS( delta ) READ( val ) END_FN
    FN( wiggle,      float ) HARNESS( wavelength ) HARNESS( amplitude ) HARNESS( phase ) HARNESS( wiggliness ) END_FN
    FN( slider_float, float ) READ( label ) READ( description ) READ( min ) READ( max ) READ( default_value ) READ( step ) fn->value = fn->default_value; END_FN
    FN( range_slider_float, interval_float ) READ( label ) READ( description ) READ( min ) READ( max ) READ( default_value ) READ( step ) fn->value = fn->default_value; END_FN

    // harness int functions
    FN( adder_int,  int ) HARNESS( r ) END_FN
    FN( slider_int, int ) READ( label ) READ( description ) READ( min ) READ( max ) READ( default_value ) READ( step ) fn->value = fn->default_value; END_FN
    FN( range_slider_int, interval_int ) READ( label ) READ( description ) READ( min ) READ( max ) READ( default_value ) READ( step ) fn->value = fn->default_value; END_FN

    // special case for menu
    FN( menu_int, int ) 
        READ( label ) 
        READ( description ) 
        READ( default_choice )
        READ( tool )
        READ( affects_widget_groups )
        READ( rerender ) 
        fn->choice = fn->default_choice;
        if( j.contains( "items" ) ) for( std::string item : j[ "items" ] ) fn->add_item( item );
    END_FN

   FN( menu_string, string ) 
        READ( label ) 
        READ( description ) 
        READ( default_choice ) 
        READ( tool ) 
        READ( affects_widget_groups ) 
        READ( rerender ) 
        fn->choice = fn->default_choice;
        if( j.contains( "items" ) ) for( std::string item : j[ "items" ] ) fn->add_item( item );
    END_FN


    // harness vec2f functions
    FN( adder_vec2f, vec2f  ) HARNESS( r ) END_FN
    FN( ratio_vec2f, vec2f  ) HARNESS( r ) END_FN
    FN( mouse_pos_fn, vec2f ) END_FN

    // harness vec2i functions
    FN( adder_vec2i, vec2i  ) HARNESS( r ) END_FN
    FN( mouse_pix_fn, vec2i ) END_FN

    // harness frgb functions
    FN( adder_frgb, frgb ) HARNESS( r ) END_FN

    // harness ucolor functions
    FN( adder_ucolor, ucolor ) HARNESS( r ) END_FN

    // pickers
    FN( direction_picker_4, direction4 ) READ( label ) READ( description ) READ( default_value ) fn->value = fn->default_value; END_FN
    FN( direction_picker_8, direction8 ) READ( label ) READ( description ) READ( default_value ) fn->value = fn->default_value; END_FN
    FN( multi_direction8_picker, int )   READ( label ) READ( description ) READ( default_value ) fn->value = fn->default_value; END_FN
    FN( box_blur_picker, box_blur_type ) READ( label ) READ( description ) READ( default_value ) fn->value = fn->default_value; END_FN

    // special case for custom_blur_picker
    FN( custom_blur_picker, int ) 
        READ( label ) 
        READ( description ) 
        // create list of pairs of multi_direction8_pickers
        if( j.contains( "pickers" ) ) {
            for( auto& p : j[ "pickers" ] ) {
                fn->add_pickers( p[ 0 ], p[ 1 ] );
            }
        }
        else {
            fn->add_pickers( 17, 17 ); // default orthogonal box blur
            fn->add_pickers( 68, 68 ); 
        }
    END_FN

    // harness bool functions
    FN( random_fn, bool ) HARNESS( p ) END_FN
    FN( random_sticky_fn, bool ) HARNESS( p_start ) HARNESS( p_change_true ) HARNESS( p_change_false ) END_FN
   // equality functions
    FN( equal_float_fn,      bool ) HARNESS( a ) HARNESS( b ) END_FN
    FN( equal_vec2f_fn,      bool ) HARNESS( a ) HARNESS( b ) END_FN
    FN( equal_int_fn,        bool ) HARNESS( a ) HARNESS( b ) END_FN
    FN( equal_vec2i_fn,      bool ) HARNESS( a ) HARNESS( b ) END_FN
    FN( equal_frgb_fn,       bool ) HARNESS( a ) HARNESS( b ) END_FN
    FN( equal_ucolor_fn,     bool ) HARNESS( a ) HARNESS( b ) END_FN
    FN( equal_string_fn,     bool ) HARNESS( a ) HARNESS( b ) END_FN
    FN( equal_bool_fn,       bool ) HARNESS( a ) HARNESS( b ) END_FN
    FN( equal_direction4_fn, bool ) HARNESS( a ) HARNESS( b ) END_FN
    FN( equal_direction8_fn, bool ) HARNESS( a ) HARNESS( b ) END_FN


    // ui functions
    FN( switch_fn, bool ) READ( tool ) READ( label ) READ( description ) READ( default_value ) READ( affects_widget_groups ) fn->value = fn->default_value; END_FN
    // special case for widget switch
    FN( widget_switch_fn, bool ) READ( switcher ) READ( widget ) READ( label ) READ( description )  END_FN

    // parameter functions
    FN( index_param_float, float ) PARAM( float ) END_FN
    FN( scale_param_float, float ) PARAM( float ) END_FN
    FN( time_param_float,  float ) PARAM( float ) END_FN

    // single field modifiers
    GEN_FN( orientation_gen_fn ) HARNESS( orientation ) END_FN
    GEN_FN( scale_gen_fn       ) HARNESS( scale )       END_FN
    GEN_FN( rotation_gen_fn    ) HARNESS( r )           END_FN
    GEN_FN( position_gen_fn    ) HARNESS( position )    END_FN

    // generalized functions (alphabetical order)
    GEN_FN( advect_element ) HARNESS( flow ) HARNESS( step ) READ( proportional ) READ( orientation_sensitive ) END_FN
    GEN_FN( angle_branch ) READ( interval ) READ( offset ) READ( mirror_offset ) HARNESS( size_prop ) HARNESS( branch_ang ) HARNESS( branch_dist ) END_FN
    GEN_FN( curly ) HARNESS( curliness ) END_FN
    // position_list should go here - figure out how to work the vector of positions

    // condition functions
    COND_FN( switch_condition ) READ( tool ) READ( label ) READ( description ) READ( default_value ) READ( affects_widget_groups ) fn->value = fn->default_value; END_FN
    COND_FN( random_condition ) HARNESS( p ) END_FN
    COND_FN( random_sticky_condition ) HARNESS( p_start ) HARNESS( p_change_true ) HARNESS( p_change_false ) END_FN
    // equality conditions
    COND_FN( equal_float_condition )      HARNESS( a ) HARNESS( b ) END_FN
    COND_FN( equal_vec2f_condition )      HARNESS( a ) HARNESS( b ) END_FN
    COND_FN( equal_int_condition )        HARNESS( a ) HARNESS( b ) END_FN
    COND_FN( equal_vec2i_condition )      HARNESS( a ) HARNESS( b ) END_FN
    COND_FN( equal_frgb_condition )       HARNESS( a ) HARNESS( b ) END_FN
    COND_FN( equal_ucolor_condition )     HARNESS( a ) HARNESS( b ) END_FN
    COND_FN( equal_string_condition )     HARNESS( a ) HARNESS( b ) END_FN
    COND_FN( equal_bool_condition )       HARNESS( a ) HARNESS( b ) END_FN
    COND_FN( equal_direction4_condition ) HARNESS( a ) HARNESS( b ) END_FN
    COND_FN( equal_direction8_condition ) HARNESS( a ) HARNESS( b ) END_FN
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
    if( j.contains( "functions" ) )  for( std::string fname : j[ "functions"  ] ) clust.next_elem.add_function(  std::get< any_gen_fn       >( s.functions[ fname ] ) ); 
    if( j.contains( "conditions" ) ) for( std::string fname : j[ "conditions" ] ) clust.next_elem.add_condition( std::get< any_condition_fn >( s.functions[ fname ] ) ); 
}

void scene_reader::read_rule( const json& j, std::shared_ptr< CA_ucolor >& ca ) {
    std::string name, type;

    // Required fields
    if( j.contains( "name" ) )          j[ "name" ].get_to( name );  else ERROR( "CA rule name missing\n" )
    DEBUG( "Reading CA rule " + name );
    if( j.contains( "type" ) )          j[ "type" ].get_to( type );  else ERROR( "CA rule type missing\n" )

    #define RULE( _T_ )    if( type == #_T_ ) {  std::shared_ptr< _T_ > r( new _T_ ); any_rule rule( r, std::ref( *r ), std::ref( *r ), name ); ca->rule = rule;
    #define HARNESSR( _T_ ) if( j.contains( #_T_ ) ) read_any_harness( j[ #_T_ ], r-> _T_ );
    #define READR( _T_ )    if( j.contains( #_T_ ) ) read( r-> _T_, j[ #_T_ ] );
    #define END_RULE()     s.CA_rules[ name ] = rule; }

    RULE( rule_life_ucolor )       READR( use_threshold ) HARNESSR( threshold ) END_RULE()
    RULE( rule_random_copy_ucolor ) END_RULE()
    RULE( rule_random_mix_ucolor )  END_RULE()
    RULE( rule_box_blur_ucolor ) HARNESSR( max_diff ) HARNESSR( bug_mode ) HARNESSR( blur_method ) HARNESSR( random_copy ) READR( custom_picker ) END_RULE()
    RULE( rule_diffuse_ucolor)      END_RULE()
    RULE( rule_gravitate_ucolor )  HARNESSR( direction ) END_RULE()
    RULE( rule_snow_ucolor )       HARNESSR( direction ) END_RULE()
    RULE( rule_pixel_sort_ucolor ) HARNESSR( direction ) HARNESSR( max_diff ) END_RULE()
    RULE( rule_funky_sort_ucolor ) HARNESSR( direction ) HARNESSR( max_diff ) READR( dafunk_l ) READR( dafunk_r ) READR( dafunk_d ) READR( hood ) END_RULE()
    DEBUG( "CA rule " + name + " complete" )
}

void scene_reader::read_effect( const json& j ) {
    std::string name, type, buf_name;

    // Required fields
    if( j.contains( "name" ) )          j[ "name" ].get_to( name );  else ERROR( "Effect name missing\n" )
    DEBUG( "Reading effect " + name )
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
        if( j.contains( "target") ) {
            j[ "target" ].get_to( buf_name );
            CA_targets[ name ] = buf_name;
        }
        READE( targeted )
        HARNESSE( p ) HARNESSE( edge_block ) HARNESSE( alpha_block ) 
        HARNESSE( bright_block ) HARNESSE( bright_range )
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

    EFF( eff_chooser )
    if( j.contains( "effects") )
    {
        for( std::string eff_name : j[ "effects" ] ) {
            if( s.effects.contains( eff_name ) ) {
                e->add_effect( s.effects[ eff_name ] );
                DEBUG( "chooser effect " + name + " adding effect " + eff_name )
            }
            else ERROR( "eff_chooser effect not found\n" )
        }
    }
    DEBUG( "chooser effect " + name + " has " + std::to_string( e->effects.size() ) + " effects" )
    // else empty effect list - should be automatic
    HARNESSE( choice )
    END_EFF()

    EFF( eff_identity ) END_EFF()
    EFF( eff_fill_frgb )   HARNESSE( fill_color ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_fill_ucolor ) HARNESSE( fill_color ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_fill_vec2i )  HARNESSE( fill_color ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_fill_vec2f )  HARNESSE( fill_color ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_fill_int )    HARNESSE( fill_color ) READE( bounded ) HARNESSE( bounds ) END_EFF()

    EFF( eff_grayscale_frgb )   END_EFF()
    EFF( eff_grayscale_ucolor ) END_EFF()

    EFF( eff_invert_ucolor )  END_EFF()
    EFF( eff_invert_frgb )    END_EFF()

    EFF( eff_rotate_colors_frgb ) HARNESSE( r ) END_EFF()
    EFF( eff_rotate_colors_ucolor ) HARNESSE( r ) END_EFF()

    EFF( eff_crop_circle_frgb )   HARNESSE( background ) HARNESSE( ramp_width ) END_EFF()
    EFF( eff_crop_circle_ucolor ) HARNESSE( background ) HARNESSE( ramp_width ) END_EFF()
    EFF( eff_crop_circle_vec2i )  HARNESSE( background ) HARNESSE( ramp_width ) END_EFF()
    EFF( eff_crop_circle_vec2f )  HARNESSE( background ) HARNESSE( ramp_width ) END_EFF()
    EFF( eff_crop_circle_int )    HARNESSE( background ) HARNESSE( ramp_width ) END_EFF()

    EFF( eff_mirror_frgb)   READE( reflect_x ) READE( reflect_y ) READE( top_to_bottom ) READE( left_to_right ) HARNESSE( center ) READE( extend ) END_EFF()
    EFF( eff_mirror_ucolor) READE( reflect_x ) READE( reflect_y ) READE( top_to_bottom ) READE( left_to_right ) HARNESSE( center ) READE( extend ) END_EFF()
    EFF( eff_mirror_vec2i)  READE( reflect_x ) READE( reflect_y ) READE( top_to_bottom ) READE( left_to_right ) HARNESSE( center ) READE( extend ) END_EFF()
    EFF( eff_mirror_vec2f)  READE( reflect_x ) READE( reflect_y ) READE( top_to_bottom ) READE( left_to_right ) HARNESSE( center ) READE( extend ) END_EFF()
    EFF( eff_mirror_int)    READE( reflect_x ) READE( reflect_y ) READE( top_to_bottom ) READE( left_to_right ) HARNESSE( center ) READE( extend ) END_EFF()

    EFF( eff_turn_frgb )   READE( direction ) END_EFF()
    EFF( eff_turn_ucolor ) READE( direction ) END_EFF()
    EFF( eff_turn_vec2i )  READE( direction ) END_EFF()
    EFF( eff_turn_vec2f )  READE( direction ) END_EFF()
    EFF( eff_turn_int )    READE( direction ) END_EFF()

    EFF( eff_flip_frgb )   READE( flip_x ) READE( flip_y ) END_EFF()
    EFF( eff_flip_ucolor ) READE( flip_x ) READE( flip_y ) END_EFF()
    EFF( eff_flip_vec2i )  READE( flip_x ) READE( flip_y ) END_EFF()
    EFF( eff_flip_vec2f )  READE( flip_x ) READE( flip_y ) END_EFF()
    EFF( eff_flip_int )    READE( flip_x ) READE( flip_y ) END_EFF()

    EFF( eff_noise_frgb )   HARNESSE( a ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_noise_ucolor ) HARNESSE( a ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_noise_vec2i )  HARNESSE( a ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_noise_vec2f )  HARNESSE( a ) READE( bounded ) HARNESSE( bounds ) END_EFF()
    EFF( eff_noise_int )    HARNESSE( a ) READE( bounded ) HARNESSE( bounds ) END_EFF()

    EFF( eff_vector_warp_frgb )   HARNESSE( vf_name ) HARNESSE( step ) HARNESSE( smooth ) HARNESSE( relative ) HARNESSE( extend ) END_EFF()
    EFF( eff_vector_warp_ucolor ) HARNESSE( vf_name ) HARNESSE( step ) HARNESSE( smooth ) HARNESSE( relative ) HARNESSE( extend ) END_EFF()
    EFF( eff_vector_warp_vec2i )  HARNESSE( vf_name ) HARNESSE( step ) HARNESSE( smooth ) HARNESSE( relative ) HARNESSE( extend ) END_EFF()
    EFF( eff_vector_warp_vec2f )  HARNESSE( vf_name ) HARNESSE( step ) HARNESSE( smooth ) HARNESSE( relative ) HARNESSE( extend ) END_EFF()
    EFF( eff_vector_warp_int )    HARNESSE( vf_name ) HARNESSE( step ) HARNESSE( smooth ) HARNESSE( relative ) HARNESSE( extend ) END_EFF()

    EFF( eff_feedback_frgb )   HARNESSE( wf_name ) END_EFF()
    EFF( eff_feedback_ucolor ) HARNESSE( wf_name ) END_EFF()
    EFF( eff_feedback_vec2i )  HARNESSE( wf_name ) END_EFF()
    EFF( eff_feedback_vec2f )  HARNESSE( wf_name ) END_EFF()
    EFF( eff_feedback_int )    HARNESSE( wf_name ) END_EFF()

    // vector field effects
    EFF( eff_complement_vec2f ) END_EFF()
    EFF( eff_radial_vec2f ) END_EFF()
    EFF( eff_cartesian_vec2f ) END_EFF()
    EFF( eff_rotate_vectors_vec2f ) HARNESSE( angle ) END_EFF()
    EFF( eff_scale_vectors_vec2f ) HARNESSE( scale ) END_EFF()
    EFF( eff_normalize_vec2f ) END_EFF()
    EFF( eff_inverse_vec2f ) HARNESSE( diameter ) HARNESSE( soften ) END_EFF()
    EFF( eff_inverse_square_vec2f ) HARNESSE( diameter ) HARNESSE( soften ) END_EFF()
    EFF( eff_concentric_vec2f ) HARNESSE( center ) END_EFF()
    EFF( eff_rotational_vec2f ) HARNESSE( center ) END_EFF()
    EFF( eff_spiral_vec2f ) HARNESSE( center ) HARNESSE( angle ) END_EFF()
    EFF( eff_vortex_vec2f ) HARNESSE( diameter ) HARNESSE( soften ) HARNESSE( intensity ) HARNESSE( center_orig ) 
                            READE( revolving ) HARNESSE( velocity ) HARNESSE( center_of_revolution ) END_EFF()
    EFF( eff_turbulent_vec2f ) HARNESSE( n ) HARNESSE( bounds ) HARNESSE( scale_factor ) 
                               HARNESSE( min_diameter ) HARNESSE( max_diameter ) 
                               HARNESSE( min_soften ) HARNESSE( max_soften )
                               HARNESSE( min_intensity ) HARNESSE( max_intensity ) READE( intensity_direction )
                               READE( revolving ) HARNESSE( min_velocity ) HARNESSE( max_velocity )
                               READE( velocity_direction ) HARNESSE( min_orbital_radius ) HARNESSE( max_orbital_radius ) END_EFF()
    EFF( eff_kaleidoscope_vec2f ) HARNESSE( center ) HARNESSE( segments ) HARNESSE( offset_angle ) HARNESSE( spin_angle ) HARNESSE( reflect ) END_EFF()                    
    EFF( eff_position_fill_vec2f ) END_EFF()

    // warp field effects
    EFF( eff_fill_warp_int ) HARNESSE( vf_name ) HARNESSE( relative ) HARNESSE( extend ) END_EFF()

    DEBUG( "Finished reading effect " + name )
}

void scene_reader::read_queue( const json& j ) {
    std::string name = "default";
    if( j.contains( "name"           ) ) read( name,           j[ "name"         ] );
    bool self_generated = false;
    if( j.contains( "self_generated" ) ) read( self_generated, j[ "self_generated" ] );
    vec2i dim( 512, 512 );
    if( j.contains( "dim"            ) ) read( dim,            j[ "dim"          ] );
    pixel_type ptype = pixel_type::PIXEL_UCOLOR;
    if( j.contains( "type"           ) ) read( ptype,          j[ "type"         ] );
    render_mode rmode = render_mode::MODE_STATIC;
    if( j.contains( "mode"           ) ) read( rmode,          j[ "mode"         ] );
    float relative_dim = 1.0;
    if( j.contains( "relative_dim"   ) ) read( relative_dim,   j[ "relative_dim" ] );  
    
    effect_list elist( name, self_generated, "none", dim, ptype, rmode, relative_dim );
    // DEBUG( "Reading queue " + .name )
    if( j.contains( "effects"      ) ) for( std::string eff_name : j[ "effects"      ] ) elist.effects.push_back( eff_name );  ;
    if( j.contains( "source"       ) ) read_harness( j[ "source" ], elist.source_name );
    // DEBUG( "queue source successfully read " )
    s.queue.push_back( elist );
    s.buffers[ name ] = elist.buf;
}

void scene_reader::read_widget_group( const json& j ) {
//    std::shared_ptr< widget_group > wg = std::make_shared< widget_group >();
    widget_group wg;

    if( j.contains( "name" ) ) read( wg.name, j[ "name" ] );
    else ERROR( "Widget group name missing\n" )
    if( j.contains( "label" ) ) read( wg.label, j[ "label" ] );
    if( j.contains( "description" ) ) read( wg.description, j[ "description" ] );
    if( j.contains( "conditions" ) ) for( std::string condition : j[ "conditions" ] ) wg.add_condition( condition );
    if( j.contains( "widgets" ) ) for( std::string widget : j[ "widgets" ] ) wg.add_widget( widget );
    s.ui.widget_groups.push_back( wg );
}

void to_json( nlohmann::json& j, const interval_int& i ) {
    j = nlohmann::json{
        i.min, i.max
    };
}

void to_json( nlohmann::json& j, const interval_float& i ) {
    j = nlohmann::json{
        i.min, i.max
    };
}

void to_json(nlohmann::json& j, const direction4& d ) {
    switch (d) {
        case direction4::D4_UP:
            j = "up";
            break;
        case direction4::D4_RIGHT:
            j = "right";
            break;
        case direction4::D4_DOWN:
            j = "down";
            break;
        case direction4::D4_LEFT:
            j = "left";
            break;
        default:
            // Handle unexpected direction4 value
            j = nullptr; // Or any indication of an error/invalid value
            break;
    }
}

void to_json(nlohmann::json& j, const direction8& d) {
    switch (d) {
        case direction8::D8_UP:
            j = "up";
            break;
        case direction8::D8_UPRIGHT:
            j = "up_right";
            break;
        case direction8::D8_RIGHT:
            j = "right";
            break;
        case direction8::D8_DOWNRIGHT:
            j = "down_right";
            break;
        case direction8::D8_DOWN:
            j = "down";
            break;
        case direction8::D8_DOWNLEFT:
            j = "down_left";
            break;
        case direction8::D8_LEFT:
            j = "left";
            break;
        case direction8::D8_UPLEFT:
            j = "up_left";
            break;
        default:
            // Handle unexpected direction8 value
            j = nullptr; // Or any indication of an error/invalid value
            break;
    }
}

void to_json( nlohmann::json& j, const box_blur_type& b ) {
    switch (b) {
        case box_blur_type::BB_ORTHOGONAL:
            j = "orthogonal";
            break;
        case box_blur_type::BB_DIAGONAL:
            j = "diagonal";
            break;
        case box_blur_type::BB_ALL:
            j = "all";
            break;
        case box_blur_type::BB_CUSTOM:
            j = "custom";
            break;
        default:
            // Handle unexpected box_blur_method value
            j = nullptr; // Or any indication of an error/invalid value
            break;
    }
}

void to_json( nlohmann::json& j, const menu_type& m ) {
    switch (m) {
        case MENU_PULL_DOWN:
            j = "pull_down";
            break;
        case MENU_RADIO:
            j = "radio";
            break;
        case MENU_IMAGE:
            j = "image";
            break;
        default:
            // Handle unexpected menu_type value
            j = nullptr; // Or any indication of an error/invalid value
            break;
    }
}
void to_json( nlohmann::json& j, const switch_fn& s) {
    j = nlohmann::json{
        {"label", s.label},
        {"description", s.description},
        {"value", s.value},
        {"default_value", s.default_value},
        {"tool", s.tool == SWITCH_SWITCH ? "switch" : "checkbox"}
    };
}

void to_json( nlohmann::json& j, const widget_group& wg ) {
    j = nlohmann::json{
        { "name", wg.name},
        { "label", wg.label},
        { "description", wg.description},
        { "conditions", wg.conditions},
        { "widgets", wg.widgets}
    };
}

/*
void to_json(nlohmann::json& j, const std::vector<widget_group>& wg_vec) {
    for (const auto& wg : wg_vec) {
        nlohmann::json j_wg;
        to_json(j_wg, wg); // Serialize each widget_group to JSON
        j.push_back(j_wg); // Add it to the JSON array
    }
}
*/
void to_json( nlohmann::json& j, const any_function& af ) {
    std::visit(overloaded{ 
        [&]( const any_fn< bool >& wrapper ) {
            std::visit(overloaded{
                [&]( const std::shared_ptr< switch_fn >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "switch_fn"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"value", fn->value},
                        {"default_value", fn->default_value},
                        {"tool", fn->tool == SWITCH_SWITCH ? "switch" : "checkbox"},
                        {"affects_widget_groups", fn->affects_widget_groups}
                    };
                },
                [&]( const std::shared_ptr< widget_switch_fn >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "widget_switch_fn"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"switcher", fn->switcher},
                        {"widget", fn->widget}
                    };
                },
                [&]( const auto& fn ) {
                    // Placeholder for other types
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "unimplemented bool function"} // Replace with actual type identification if needed
                        // Other placeholder fields...
                    };
                }
            }, wrapper.any_fn_ptr);
        },
        [&]( const any_fn< int >& wrapper ) {
            std::visit(overloaded{
                [&]( const std::shared_ptr< slider_int >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "slider_int"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"min", fn->min},
                        {"max", fn->max},
                        {"default_value", fn->default_value},
                        {"step", fn->step},
                        {"value", fn->value}
                    };
                },
                [&]( const std::shared_ptr< menu_int >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "menu_int"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"choice", fn->choice},
                        {"default_choice", fn->default_choice},
                        {"tool", fn->tool},
                        {"items", fn->items},
                        {"affects_widget_groups", fn->affects_widget_groups},
                        {"rerender", fn->rerender}
                    };
                },
                [&]( const std::shared_ptr< multi_direction8_picker >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "multi_direction_picker"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"value", fn->value},
                        {"default_value", fn->default_value}
                    };
                },
                [&]( const std::shared_ptr< custom_blur_picker >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "custom_blur_picker"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"pickers", fn->pickers}
                    };
                },
                [&]( const auto& fn ) {
                    // Placeholder for other types
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "unimplemented json output for int function"} // Replace with actual type identification if needed
                        // Other placeholder fields...
                    };
                }
            }, wrapper.any_fn_ptr);
        },
        [&]( const any_fn< float >& wrapper ) {
            std::visit(overloaded{
                [&]( const std::shared_ptr< slider_float >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "slider_float"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"min", fn->min},
                        {"max", fn->max},
                        {"default_value", fn->default_value},
                        {"step", fn->step},
                        {"value", fn->value}
                    };
                },
                [&]( const auto& fn ) {
                    // Placeholder for other types
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "unimplemented float function"} // Replace with actual type identification if needed
                        // Other placeholder fields...
                    };
                }
            }, wrapper.any_fn_ptr);
        },
        [&]( const any_fn< interval_int >& wrapper ) {
            std::visit(overloaded{
                [&]( const std::shared_ptr< range_slider_int >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "range_slider_int"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"min", fn->min},
                        {"max", fn->max},
                        {"default_value", fn->default_value},
                        {"step", fn->step},
                        {"value", fn->value}
                    };
                },
                [&]( const auto& fn ) {
                    // Placeholder for other types
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "unimplemented interval_int function"} // Replace with actual type identification if needed
                        // Other placeholder fields...
                    };
                }
            }, wrapper.any_fn_ptr);
        },
        [&]( const any_fn< interval_float >& wrapper ) {
            std::visit( overloaded {
                [&]( const std::shared_ptr< range_slider_float >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "range_slider_float"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"min", fn->min},
                        {"max", fn->max},
                        {"default_value", fn->default_value},
                        {"step", fn->step},
                        {"value", fn->value}
                    };
                },
                [&]( const auto& fn ) {
                    // Placeholder for other types
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "unimplemented interval_float function"} // Replace with actual type identification if needed
                        // Other placeholder fields...
                    };
                }
            }, wrapper.any_fn_ptr );
        },
        /*
        [&]( const any_fn< vec2f >& wrapper ) {
        },
        [&]( const any_fn< vec2i >& wrapper ) {
        },
        [&]( const any_fn< frgb >& wrapper ) {
        },
        [&]( const any_fn< ucolor >& wrapper ) {
        },
        */
        [&]( const any_fn< std::string >& wrapper ) {
            std::visit( overloaded {
                [&]( const std::shared_ptr< menu_string >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "menu_string"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"choice", fn->choice},
                        {"default_choice", fn->default_choice},
                        {"tool", fn->tool},
                        {"items", fn->items},
                        {"affects_widget_groups", fn->affects_widget_groups},
                        {"rerender", fn->rerender}

                    };
                },
                [&]( const auto& fn ) {
                    // Placeholder for other types
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "unimplemented string function"} // Replace with actual type identification if needed
                        // Other placeholder fields...
                    };
                },
            }, wrapper.any_fn_ptr);  
        },
        [&]( const any_fn< direction4 >& wrapper ) {
            std::visit( overloaded {
                [&]( const std::shared_ptr< direction_picker_4 >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "direction_picker_4"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"value", fn->value},
                        {"default_value", fn->default_value}
                    };
                },
                [&]( const auto& fn ) {
                    // Placeholder for other types
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "unimplemented direction4 function"} // Replace with actual type identification if needed
                        // Other placeholder fields...
                    };
                }
            }, wrapper.any_fn_ptr);
        },
        [&]( const any_fn< direction8 >& wrapper ) {
            std::visit( overloaded {
                [&]( const std::shared_ptr< direction_picker_8 >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "direction_picker_8"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"value", fn->value},
                        {"default_value", fn->default_value}
                    };
                },
                [&]( const auto& fn ) {
                    // Placeholder for other types
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "unimplemented direction8 function"} // Replace with actual type identification if needed
                        // Other placeholder fields...
                    };
                }
            }, wrapper.any_fn_ptr );
        },
        [&]( const any_fn< box_blur_type >& wrapper ) {
            std::visit( overloaded {
                [&]( const std::shared_ptr< box_blur_picker >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "box_blur_picker"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"value", fn->value},
                        {"default_value", fn->default_value}
                    };
                },
                [&]( const auto& fn ) {
                    // Placeholder for other types
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "unimplemented box_blur_type function"} // Replace with actual type identification if needed
                        // Other placeholder fields...
                    };
                }
            }, wrapper.any_fn_ptr );
        },
        /* 
        [&]( const any_gen_fn& wrapper ) {
        },
        */
        [&]( const any_condition_fn& wrapper ) {
             std::visit(overloaded{
                [&]( const std::shared_ptr<switch_condition>& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "switch_condition"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"value", fn->value},
                        {"default_value", fn->default_value},
                        {"tool", fn->tool == SWITCH_SWITCH ? "switch" : "checkbox"},
                        {"affects_widget_groups", fn->affects_widget_groups}
                    };
                },
                [&]( const std::shared_ptr< widget_switch_condition >& fn ) {
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "widget_switch_condition"},
                        {"label", fn->label},
                        {"description", fn->description},
                        {"switcher", fn->switcher},
                        {"widget", fn->widget}
                    };
                },
                [&]( const auto& fn ) {
                    // Placeholder for other types
                    j = nlohmann::json{
                        {"name", wrapper.name},
                        {"type", "unimplemented function"} // Replace with actual type identification if needed
                        // Other placeholder fields...
                    };
                }
            }, wrapper.my_condition_fn );           
        },
        [&]( auto& wrapper ) {
            // Placeholder for other types
            j = nlohmann::json{
                {"name", "unimplemented return type"},
            };
        }
    }, af);
}

/*
std::string scene_writer::write_UI_json() {

}
*/
