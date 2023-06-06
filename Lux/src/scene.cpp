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
template< class T > void splat_element( std::shared_ptr< buffer_pair< T > > target_buf, element& el ) {
    std::cout << " splat_element " << std::endl; 
    typedef std::shared_ptr< buffer_pair< T > > buf_ptr;  

    if( std::holds_alternative< buf_ptr >( el.img ) ) { // Check if element image is same type as target. Otherwise no effect. Future - splat image of different type.
        buf_ptr img_buf  = std::get< buf_ptr >( el.img ); 
        std::cout << "image pointer " << img_buf << std::endl;
        std::cout << "splat_element() - matching image buffer" << std::endl;
        if( img_buf.get() ) {                // Check if image buffer is not null   
            if( img_buf->has_image() ) {
                //std::cout << "has image" << std::endl;
                //image< T >& img( img_buf->get_image() ); // initialize reference to image
                std::optional< std::reference_wrapper< image< T > > > mask = std::nullopt;
                std::optional< T > tint = std::nullopt;

                /*
                image< T > splat( vec2i( 512, 512 ) );
                T w;
                white( w );
                splat.fill( w );
                splat.crop_circle();
                */

                if( std::holds_alternative< buf_ptr >( el.mask ) ) {
                    buf_ptr mask_buf = std::get< buf_ptr >( el.mask );
                    if( mask_buf.get() ) {
                        if( mask_buf->has_image() ) {
                            std::cout << "has mask image" << std::endl;
                            mask = std::ref( mask_buf->get_image() );
                        }
                    }
                }

                // Future - splat mask of different type.
                if( el.tint.has_value() ) if( std::holds_alternative< T >( *(el.tint) ) ) tint = std::get< T >( *(el.tint) );
                float th = el.rotation;
                if( el.orientation_lock ) th += el.orientation;
                std::cout << "element - position: " << el.position.x << " " << el.position.y << " scale: " << el.scale << " rotation: " << el.rotation << " orientation: " << el.orientation << " orientation_lock: " << el.orientation_lock << std::endl;

                //std::cout << " prepared to splat\n";
                //image_ptr target( target_buf->get_image_ptr() ); // Initialize pointer to target image
    //           if( target_buf->has_image() ) target_buf->get_image().splat( el.position, el.scale, th, img_buf->get_image(), mask, tint, el.mmode ); 
                if( target_buf->has_image() ) {
                    std::cout << "target bounds: ";
                    target_buf->get_image().get_bounds().print();
                    std::cout << "target ipbounds: ";
                    target_buf->get_image().get_ipbounds().print();
                    std::cout << "target size : " << target_buf->get_image().size() << std::endl;
                    std::cout << "splat size  : " << img_buf->get_image().size() << std::endl;
                    target_buf->get_image().splat( el.position, el.scale, th, img_buf->get_image(), mask, tint, el.mmode ); 
                    //target_buf->get_image().copy( splat );
                }
                //std::cout << "splat complete\n";
            }
            else std::cout << "splat_element() - no image in buffer" << std::endl;
        }
        else std::cout << "splat_element() - null image buffer" << std::endl;
    }
    else std::cout << "splat_element() - unmatched image buffer" << std::endl;
}

// splats any element onto any image
void element::render( any_buffer_pair_ptr& target ) { 
    std::cout << " element::render\n";
    pixel_type ptype = ( pixel_type )target.index();
    std::cout << " pixel type " << ptype << std::endl;
    switch( ( pixel_type )target.index() ) {     // replace with std::visit
        case( PIXEL_FRGB   ): splat_element< frgb   >( std::get< std::shared_ptr< buffer_pair< frgb > > >( target ), *this ); break;
        case( PIXEL_UCOLOR ): splat_element< ucolor >( std::get< std::shared_ptr< buffer_pair< ucolor > > >( target ), *this ); break;
        case( PIXEL_VEC2F  ): splat_element< vec2f  >( std::get< std::shared_ptr< buffer_pair< vec2f > > >( target ), *this ); break;
        case( PIXEL_INT    ): splat_element< int    >( std::get< std::shared_ptr< buffer_pair< int > > >( target ), *this ); break;
        case( PIXEL_VEC2I  ): splat_element< vec2i  >( std::get< std::shared_ptr< buffer_pair< vec2i > > >( target ), *this ); break;
    }
}

void element::operator () ( any_buffer_pair_ptr& buf, element_context& context ) {
    std::cout << " element::operator ()" << std::endl;
    render( buf );
    std::cout << " element::operator () complete" << std::endl;
}

// Recursively generate and render elements
void cluster::render( scene& s, any_buffer_pair_ptr& buf ) { 
    // The lambda function uses a generic lambda with auto as the argument type, and returns the same type as its argument. 
    // The std::visit function will invoke this lambda function with the correct argument based on the type stored in the variant, 
    // and will return the value returned by the lambda function. Note that the decltype(ptr) return type is used to preserve the 
    // exact type of the stored object. (Generated by ChatGPT)
    element el = root_elem;
    element_context context( el, *this, s, buf );
    //std::cout << " context created" << std::endl;
    // If next_element contains no functions, simply render initial element
    if( next_elem.functions.size() == 0 ) el.render( buf );
    else while( next_elem( context ) ) {
        el.render( buf ); 
        //std::cout << "rendered element " << el.index << std::endl; 
    }
    //( ( fimage & )img ).write_jpg( "hk_cluster.jpg", 100 ); // debug - save frame after each cluster                   
}

// render into a buffer pair. Rendering modifies image directly - does not require buffer swap.
// in this case the cluster serves as an effect functor (the effect being rendering)
void cluster::operator () ( any_buffer_pair_ptr& buf, element_context& context ) {
    render( context.s, context.buf ); // does not require buffer swap
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

void effect_list::resize( const vec2i& new_dim ) {
    // need to resize buffer (retaining data) instead?
    std::visit( [&]( auto& b ) { b->reset( new_dim ); }, buf );       
}

void effect_list::render( scene& s ) { 
    std::cout << "effect_list::render() source name " << source_name << " rendered = " << rendered << std::endl;
    if( rmode == MODE_EPHEMERAL || !rendered ) { // ephemeral buffers are re-rendered each frame
        //std::cout << "preparing to copy buffer " << source_name << std::endl;
        if( s.buffers.contains( source_name ) ) {  // load source image or result into buffer
            // Copy source buffer into working buffer 
            any_buffer_pair_ptr& source_buf = s.buffers[ source_name ];
            std::visit( [ &, source_buf ]( auto& b ) { copy_buffer( b, source_buf ); }, buf );
        }
        else {  // blank buffer
            throw std::runtime_error( "effect_list::render() - source buffer not found" );
            resize( dim );
        }
    }
    if( !( rmode == MODE_STATIC && rendered ) ) { // no need to re-render static buffers
        for( auto& e : effects ) {
            // default element and cluster
            element default_element;
            next_element default_next_element;
            cluster default_cluster( default_element, default_next_element );
            element_context context( default_element, default_cluster, s, buf );
            s.effects[ e ]( buf, context );
            std::cout << "rendered effect " << e << " into buffer " << name << std::endl;
        }
        rendered = true;
    }
    else std::cout << "static or already rendered" << std::endl;
}

void effect_list::restart( scene& s ) {
    //std::cout << "effect_list::restart()" << std::endl;
    rendered = false;
    any_buffer_pair_ptr source_buf = s.buffers[ source_name ];
    std::visit( [ source_buf ]( auto& b ) { copy_buffer( b, source_buf ); }, buf );
    //std::cout << "effect_list::restart() - buffer set" << std::endl;
}

scene::scene( float time_interval_init ) : time_interval( time_interval_init ), default_time_interval( time_interval_init ) {}

scene::scene( const std::string& filename, float time_interval_init ) 
    : time( 0.0f ), time_interval( time_interval_init ), default_time_interval( time_interval_init )
{
    scene_reader reader( *this, filename );
}

void scene::restart() {
    //std::cout << "scene::restart()" << std::endl;
    time = 0.0f;
    time_interval = default_time_interval;
    for( auto& eff_list : queue ) eff_list.restart( *this );
    //std::cout << "scene::restart() - effects restarted" << std::endl;
}

void scene::set_time_interval( const float& t ) {
    time_interval = t;
}

void scene::render() {
    //std::cout << "scene::render()" << std::endl; 
    for( auto& eff_list : queue ) eff_list.render( *this );
    time += time_interval;
    //std::cout << "scene rendered" << std::endl;
}

void scene::save_result( 
    const std::string& filename, 
    const vec2i& dim,
    pixel_type ptype, 
    file_type ftype, 
    int quality )
{ 
    // temporary - assumes result type uimage
    auto out = std::get< ubuf_ptr >( queue.back().buf );
    if( out->has_image() ) {
        //std::cout << "writing image" << std::endl;
        ((uimage *)(out->get_image_ptr().get()))->write_jpg( filename, quality );
    }
    else {
        throw std::runtime_error( "scene::save_result: no image to write" );
    }

    // the proper way something like this? Or use temlate for this function?
    //std::visit( [ & ]( auto& out ){ out->get_image().write_file( filename, ftype, quality ); }, queue.back().buf );
}

void scene::render_and_save( 
    const std::string& filename, 
    const vec2i& dim,
    pixel_type ptype, 
    file_type ftype, 
    int quality )
{ 
    //std::cout << "scene::render_and_save()" << std::endl;
    any_buffer_pair_ptr any_out;
    // bounds set automatically by image constructor
    switch( ptype ) {
        case( PIXEL_FRGB   ): any_out = std::make_shared< buffer_pair< frgb >   >( dim ); break;
        case( PIXEL_UCOLOR ): any_out = std::make_shared< buffer_pair< ucolor > >( dim ); break;
        case( PIXEL_VEC2F  ): any_out = std::make_shared< buffer_pair< vec2f >  >( dim ); break;
        case( PIXEL_INT    ): any_out = std::make_shared< buffer_pair< int >    >( dim ); break;
        case( PIXEL_VEC2I  ): any_out = std::make_shared< buffer_pair< vec2i >  >( dim ); break;
    }
    //std::cout << "Created image pointer" << std::endl;
    set_output_buffer( any_out ); // set output buffer
    render(); // render into image
    save_result( filename, dim, ptype, ftype, quality ); // save image
} 

void scene::animate( 
    std::string basename, 
    int nframes, 
    vec2i dim,
    pixel_type ptype, 
    file_type ftype, 
    int quality )
{
    any_buffer_pair_ptr any_out;
    switch( ptype ) {
        case( PIXEL_FRGB   ): any_out = std::make_shared< buffer_pair< frgb >   >( dim ); break;
        case( PIXEL_UCOLOR ): any_out = std::make_shared< buffer_pair< ucolor > >( dim ); break;
        case( PIXEL_VEC2F  ): any_out = std::make_shared< buffer_pair< vec2f >  >( dim ); break;
        case( PIXEL_INT    ): any_out = std::make_shared< buffer_pair< int >    >( dim ); break;
        case( PIXEL_VEC2I  ): any_out = std::make_shared< buffer_pair< vec2i >  >( dim ); break;
    }
    //std::cout << "Created image pointer" << std::endl;
    set_output_buffer( any_out ); // set output buffer

    //std::cout << "scene::animate" << std::endl;
    time_interval = 1.0f / nframes;
    time = 0.0f;
    for( int frame = 0; frame < nframes; frame++ ) {
        std::ostringstream s;
        s << basename << std::setfill('0') << std::setw(4) << frame << ".jpg";
        std::string filename = s.str();
        render();
        save_result( filename, dim, ptype, ftype, quality );
        time += time_interval;
        std::cout << "frame " << frame << std::endl;
    }

    // future: make the video file here 
}

void scene::set_output_buffer( any_buffer_pair_ptr& buf ) {
    //std::cout << "scene::set_output_buffer()" << std::endl;
    auto& output_list = queue.back();
    output_list.buf = buf;
    output_list.ptype = ( pixel_type )buf.index();
    vec2i dim_out;
    std::visit( [&]( auto& b ) { dim_out = b->get_image().get_dim(); }, buf );
    for( int i = 0; i < queue.size() - 1; i++ ) {
        auto& eff_list = queue[ i ];
        eff_list.rendered = false;
        vec2i dim = { std::round( dim_out.x * eff_list.relative_dim ), std::round( dim_out.y * eff_list.relative_dim ) };
        eff_list.resize( dim );
    }
}

effect_list& scene::get_effect_list( const std::string& name ) {
    for( auto& eff_list : queue )
        if( eff_list.name == name )
            return eff_list;
    throw std::runtime_error( "scene::get_effect_list: effect list" + name + " not found" );
}