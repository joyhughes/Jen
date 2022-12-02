#include "scene.hpp"
#include "scene_io.hpp"
#include "fimage.hpp"
#include "uimage.hpp"
#include "vector_field.hpp"
#include <fstream>
#include <sstream>

using json = nlohmann::json;

scene_reader::scene_reader( scene& s_init, std::string( filename ) ) : s( s_init ) {
    // read a JSON file
    std::ifstream in_stream(filename);
    json j = json::parse(in_stream); 

    // scene fields
    if( j.contains( "name" ) ) j[ "name" ].get_to( s.name ); else s.name = "Unnamed";
    if( j.contains( "size" ) ) s.size = read_vec2i( j[ "size" ] ); else s.size = { 1024, 1024 };
    if( j.contains( "images" ) )    for( auto& jimg :   j[ "images" ] )   read_image( jimg );
    // effects - TBI
    //if( j.contains( "effects" ) ) for( auto& jeff : j[ "effects" ] ) read_effect( jeff );
    if( j.contains( "elements" ) )  for( auto& jelem :  j[ "elements"  ] ) read_element(  jelem  );  

    // add default conditions
    s.bool_fns[ "initial_element"   ] = initial_element;
    s.bool_fns[ "following_element" ] = following_element;
    s.bool_fns[ "top_level"         ] = top_level;
    s.bool_fns[ "lower_level"       ] = lower_level;
    
    if( j.contains( "functions" ) ) for( auto& jfunc :  j[ "functions" ] ) read_function( jfunc  );
    if( j.contains( "clusters" ) )  for( auto& jclust : j[ "clusters"  ] ) read_cluster(  jclust ); // if no clusters, blank scene
}

vec2f scene_reader::read_vec2f( const json& j ) { return vec2f( { j[0], j[1] } ); }
vec2i scene_reader::read_vec2i( const json& j ) { return vec2i( { j[0], j[1] } ); }
frgb  scene_reader::read_frgb(  const json& j ) { return frgb( j[0], j[1], j[2] ); }
bb2f  scene_reader::read_bb2f(  const json& j ) { return bb2f( read_vec2f( j[0] ), read_vec2f( j[1]) ); }
bb2i  scene_reader::read_bb2i(  const json& j ) { return bb2i( read_vec2i( j[0] ), read_vec2i( j[1]) ); }

// ucolor represented as hexidecimal string
ucolor scene_reader::read_ucolor(   const json& j ) { 
    std::string color;
    ucolor u;

    j.get_to( color );
    std::istringstream( color ) >> std::hex >> u;
    return u;
}

void scene_reader::read_image( const json& j ) {
    std::string type, name, filename;

    if( j.contains( "type") ) j[ "type" ].get_to( type );
    else throw std::runtime_error( "scene_reader::read_image error - image type missing\n" );

    if( j.contains( "filename" ) ) j[ "filename" ].get_to( filename );
    else throw std::runtime_error( "scene_reader::read_image error - image filename missing\n" );

    if( j.contains( "name" ) ) j[ "name" ].get_to( name );
    else name = filename;

    if( type == "fimage" ) {
        std::shared_ptr< fimage > fimage_ptr( new fimage( filename ) );
        s.images[ name ] = fimage_ptr;
    }

    if( type == "uimage" ) {
        std::shared_ptr< uimage > uimage_ptr( new uimage( filename ) );
        s.images[ name ] = uimage_ptr;
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

    if( j.contains( "name" ) ) j[ "name" ].get_to( name );  else throw std::runtime_error( "Element name missing\n" );
    s.elements[ name ] = std::make_shared< element >();
    element& elem = *(s.elements[ name ]);

    if( j.contains( "position" ) ) elem.position = read_vec2f( j[ "position" ] );
    if( j.contains( "scale" ) )       j[ "scale"       ].get_to( elem.scale );
    if( j.contains( "rotation" ) )    j[ "rotation"    ].get_to( elem.rotation );
    if( j.contains( "orientation" ) ) j[ "orientation" ].get_to( elem.orientation );
    if( j.contains( "orientation_lock" ) ) j[ "orientation_lock" ].get_to( elem.orientation_lock );
    if( j.contains( "mask_mode" ) )   j[ "mask_mode"   ].get_to( mask_mode );
    
    if( mask_mode == "additive" ) elem.mmode = MASK_ADDITIVE;
    if( mask_mode == "trim"     ) elem.mmode = MASK_TRIM;
    if( mask_mode == "opacity"  ) elem.mmode = MASK_OPACITY;
    if( mask_mode == "blend"    ) elem.mmode = MASK_BLEND;

    // image, mask, tint
    if( j.contains( "image" ) ) {
        j[ "image" ].get_to( img );
        elem.img = s.images[ img ];
    }

    if( j.contains( "mask" ) ) {
        j[ "mask" ].get_to( mask );
        elem.mask = s.images[ mask ];
    }

    if( j.contains( "tint" ) ) {
        auto k = j[ "tint" ];
        if( k.contains( "frgb"   ) ) elem.tint = read_frgb(   k[ "frgb" ]   );
        if( k.contains( "ucolor" ) ) elem.tint = read_ucolor( k[ "ucolor" ] );
        if( k.contains( "vec2f"  ) ) elem.tint = read_vec2f(  k[ "vec2f" ]  );
    }
}

// forward references not implemented
template< class T > void scene_reader::read_harness( const json& j, harness< T >& h, std::map< std::string, std::function< T ( T&, element_context& ) > >& harness_fns ) {
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

void scene_reader::read_function( const json& j ) {
    std::string name, type, fn_name;

    if( j.contains( "name" ) ) j[ "name" ].get_to( name );  else throw std::runtime_error( "Function name missing\n" );
    if( j.contains( "type" ) ) j[ "type" ].get_to( type );  else throw std::runtime_error( "Function type missing\n" );

    // special case for conditionals
    if( type == "filter" ) {
        auto fn = filter();
        if( j.contains( "conditions" ) ) for( auto c : j[ "conditions" ] ) fn.add_condition( s.bool_fns[ c ] );
        if( j.contains( "functions"  ) ) for( auto f : j[ "functions"  ] ) fn.add_function(  s.gen_fns[  f ] );
        s.gen_fns[ name ] = fn;
    }

    // example of expanded macro sequence
    /* if( type == "orientation_gen_fn" ) { 
        auto fn = orientation_gen_fn(); 
        if( j.contains( "orientation" ) ) read_any_harness( j[ "orientation" ], fn. orientation );
        s.functions[ name ] = fn;
    } */

    #define FN( _T_ )      if( type == #_T_ ) {  auto fn = _T_ ();  
    #define HARNESS( _T_ ) if( j.contains( #_T_ ) ) read_any_harness( j[ #_T_ ], fn. _T_ );
    #define READ( _T_ )    if( j.contains( #_T_ ) ) read( fn. _T_, j[ #_T_ ] );
    #define PARAM( _T_ )   if( j.contains( "fn" ) ) { j[ "fn" ].get_to( fn_name ); fn.fn = s. _T_##_fns[ fn_name ]; }
    #define END_FN( _T_ )  s. _T_##_fns[ name ] = fn; }

    // harness float functions
    FN( adder_float ) HARNESS( r ) END_FN( float )
    FN( log_fn      ) HARNESS( scale ) HARNESS( shift ) END_FN( float )
    FN( ratio_float ) HARNESS( r ) END_FN( float )
    FN( wiggle      ) HARNESS( wavelength ) HARNESS( amplitude ) HARNESS( phase ) HARNESS( wiggliness ) END_FN( float )

    // parameter functions
    FN( index_param ) PARAM( float ) END_FN( float )
    FN( scale_param ) PARAM( float ) END_FN( float )
    FN( time_param  ) PARAM( float ) END_FN( float )

    // single field modifiers
    FN( orientation_gen_fn ) HARNESS( orientation ) END_FN( gen )
    FN( scale_gen_fn       ) HARNESS( scale ) END_FN( gen )

    // generalized functions (alphabetical order)
    FN( advect_element ) HARNESS( flow ) HARNESS( step ) READ( proportional ) READ( orientation_sensitive ) END_FN( gen )
    FN( angle_branch ) READ( interval ) READ( offset ) READ( mirror_offset ) HARNESS( size_prop ) HARNESS( branch_ang ) HARNESS( branch_dist ) END_FN( gen )
    FN( curly ) HARNESS( curliness ) END_FN( gen )
    // position_list should go here - figure out how to work the vector of positions
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
    if( j.contains( "name" ) )          j[ "name" ].get_to( name );  else throw std::runtime_error( "Cluster name missing\n" );
    // Check for unique name. Future - make sure duplicate clusters refer to the same cluster

    if( s.clusters.contains( name ) )   throw std::runtime_error( "Cluster name collision\n" ); 
    if( j.contains( "element" ) )     j[ "element" ].get_to( root_elem_name ); else throw std::runtime_error( "Cluster root_elem missing\n" );

    // create cluster object
    s.next_elements[ name ] = std::make_shared< next_element >();
    s.clusters[ name ] = std::make_shared< cluster >( *( s.elements[ root_elem_name ] ), *( s.next_elements[ name ] ) );
    cluster& clust = *(s.clusters[ name ]);   // reference to cluster object

    // optional fields
    if( j.contains( "max_n" ) )         j[ "max_n" ].get_to( clust.max_n );
    if( j.contains( "max_depth" ) )     j[ "max_depth" ].get_to( clust.max_depth );
    if( j.contains( "min_scale" ) )     j[ "min_scale" ].get_to( clust.min_scale );
    if( j.contains( "bounds" ) )        clust.bounds = read_bb2f( j[ "bounds" ] );

    if( j.contains( "next_element" ) )  for( std::string fname : j[ "next_element" ] ) 
    {
        clust.next_elem.add_function( s.gen_fns[ fname ] ); // Empty next_element is allowed - useful for single element clusters
    }
    clust.next_elem.max_index = clust.max_n;    // Set limit to the number of elements in cluster
    if( j.contains( "tlc" ) )  { 
        j[ "tlc" ].get_to( tlc );
        if( tlc ) s.tlc.push_back( name );
    }
}
