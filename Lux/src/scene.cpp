#include "scene.hpp"
#include "next_element.hpp"
#include "fimage.hpp"
#include "uimage.hpp"
#include "vector_field.hpp"
#include "warp_field.hpp"
#include "offset_field.hpp"
#include "scene_io.hpp"
#include <optional>
#include <sstream>

#define SCENE_DEBUG

// splats any element onto a particular image
template< class T > void splat_element( std::shared_ptr< image< T > > target, element& el ) {
    //std::cout << " splat_element" << std::endl; 
    typedef std::shared_ptr< image< T > > image_ptr;    

    image_ptr img = NULL;
    image_ptr mask = NULL;
    std::optional< T > tint = std::nullopt;
    
    if( std::holds_alternative< image_ptr >( el.img ) ) { 
        img  = std::get< image_ptr >( el.img ); 
        //std::cout << "Element holds image alternative\n";
    }
    else {
        //std::cout << "Element does not hold image alternative\n";
    }
    if( std::holds_alternative< image_ptr >( el.mask ) ) mask = std::get< image_ptr >( el.mask );
    if( el.tint.has_value() ) { 
        if( std::holds_alternative< T >( *(el.tint) ) )   tint = std::get< T >( *(el.tint) );
    }

    if( !img.get() ) std::cout << "Apparent null image pointer\n";
    float th = el.rotation;
    if( el.orientation_lock ) th += el.orientation;

    //std::cout << " prepared to splat\n";
    if( target.get() ) target->splat( el.position, el.scale, th, img, mask, tint, el.mmode ); 
    else std::cout << "error - no target image\n";
    //std::cout << "splat complete\n";
}

// splats any element onto any image
void element::render( any_image_ptr& target, const float& t ) { 
    //std::cout << " element::render\n";
    pixel_type ptype = ( pixel_type )target.index();
    //std::cout << " pixel type " << ptype << std::endl;
    switch( ( pixel_type )target.index() ) {
        case( PIXEL_FRGB   ): splat_element< frgb   >( std::get< fimage_ptr >( target ), *this ); break;
        case( PIXEL_UCOLOR ): splat_element< ucolor >( std::get< uimage_ptr >( target ), *this ); break;
        case( PIXEL_VEC2F  ): splat_element< vec2f  >( std::get< vfield_ptr >( target ), *this ); break;
        case( PIXEL_INT    ): splat_element< int    >( std::get< wfield_ptr >( target ), *this ); break;
        case( PIXEL_VEC2I  ): splat_element< vec2i  >( std::get< ofield_ptr >( target ), *this ); break;
    }
}

// needed?
// render into a buffer pair. Rendering modifies image directly - does not require buffer swap.
// in this case the element serves as an effect functor
template<class T> void element::operator()(buffer_pair<T> &buf, const float &t) {
    if( !buf.get_image() ) throw std::runtime_error( "element: null image pointer" );
    render( buf.get_image(), t ); // does not require buffer swap
}

// Recursively generate and render elements
void cluster::render( scene& s, any_image_ptr& img, const float& t, const float& time_interval ) { 
    element el = root_elem;
    element_context context( el, *this, s, img, t, time_interval );
    //std::cout << " context created" << std::endl;
    // If next_element contains no functions, simply render initial element
    if( next_elem.functions.size() == 0 ) el.render( img, t );
    else while( next_elem( context ) ) { 
        el.render( img, t ); 
        //std::cout << "rendered element " << el.index << std::endl; 
    }
    //( ( fimage & )img ).write_jpg( "hk_cluster.jpg", 100 ); // debug - save frame after each cluster                   
}

template<class T> void cluster::operator()(buffer_pair<T> &buf, const float &t) {
    if( !buf.get_image().get() ) throw std::runtime_error( "cluster: null image pointer" );
    render( buf.get_image(), t ); // does not require buffer swap
    // add logic for double buffering if background dependent and top level true
}

// change root element parameters for branching cluster
void cluster::set_root( element& el ) {
    root_elem.position = el.position;
    root_elem.scale = el.scale;
    root_elem.rotation = el.rotation;
    root_elem.orientation = el.orientation;
    root_elem.orientation_lock = el.orientation_lock;
    root_elem.derivative = el.derivative;
    root_elem.derivative_lock = el.derivative_lock;
    root_elem.index = 0;
}

/*
// render into a buffer pair. Rendering modifies image directly - does not require buffer swap.
// in this case the cluster serves as an effect functor (the effect being rendering)
void cluster::operator () ( any_buffer_pair buf, const float& t ) {
    render( buf.get_image(), t );
}
*/

scene::scene() {}

scene::scene( const std::string& filename ) 
{
    scene_reader reader( *this, filename );
}

void scene::render( any_image_ptr &any_out, const float &time, const float &time_interval ) {
    for( auto& name : tlc ) {
        clusters[ name ]->render( *this, any_out, time );    // render top level clusters in order
        //std::cout << "rendered cluster " << name << std::endl;
    }
}

void scene::render_and_save( 
    const std::string& filename, 
    const vec2i& dim,
    const float& time, 
    const float& time_interval, 
    pixel_type ptype, 
    file_type ftype, 
    int quality )
{ 
    //std::cout << "scene::render" << std::endl;
    any_image_ptr any_out;
    // bounds set automatically by image constructor
    switch( ptype ) {
        case( PIXEL_FRGB   ): any_out = std::make_shared< fimage       >( dim ); break;
        case( PIXEL_UCOLOR ): any_out = std::make_shared< uimage       >( dim ); break;
        case( PIXEL_VEC2F  ): any_out = std::make_shared< vector_field >( dim ); break;
        case( PIXEL_INT    ): any_out = std::make_shared< warp_field   >( dim ); break;
        case( PIXEL_VEC2I  ): any_out = std::make_shared< offset_field >( dim ); break;
    }
    //std::cout << "Created image pointer" << std::endl;
    // future: add pre-effects here
    render( any_out, time, time_interval ); // render into image

    // future: add after-effects here
    // future: optional write to png
    std::visit( [ & ]( auto&& out ){ out->write_file( filename, ftype, quality ); }, any_out );
} 

void scene::animate( std::string basename, int nframes, vec2i dim )
{
    //std::cout << "scene::animate" << std::endl;
    float time_interval = 1.0f / nframes;
    float time = 0.0f;
    for( int frame = 0; frame < nframes; frame++ ) {
        std::ostringstream s;
        s << basename << std::setfill('0') << std::setw(4) << frame << ".jpg";
        std::string filename = s.str();
        render_and_save( filename, dim, time, time_interval );
        time += time_interval;
        std::cout << "frame " << frame << std::endl;
    }

    // future: make the video file here 
}

