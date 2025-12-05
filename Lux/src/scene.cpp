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
    typedef std::shared_ptr< buffer_pair< T > > buf_ptr;  

    if( std::holds_alternative< buf_ptr >( el.img ) ) { // Check if element image is same type as target. Otherwise no effect. Future - splat image of different type.
        buf_ptr img_buf  = std::get< buf_ptr >( el.img ); 
        if( img_buf.get() ) {                // Check if buffer pair null   
            if( img_buf->has_image() ) {
                std::optional< std::reference_wrapper< image< T > > > mask = std::nullopt;
                std::optional< T > tint = std::nullopt;

                if( std::holds_alternative< buf_ptr >( el.mask ) ) {
                    buf_ptr mask_buf = std::get< buf_ptr >( el.mask );
                    if( mask_buf.get() ) {
                        if( mask_buf->has_image() ) {
                            mask = std::ref( mask_buf->get_image() );
                        }
                    }
                }

                // Future - splat mask of different type.
                if( el.tint.has_value() ) if( std::holds_alternative< T >( *(el.tint) ) ) tint = std::get< T >( *(el.tint) );
                float th = el.rotation;
                if( el.orientation_lock ) th += el.orientation;

                if( target_buf->has_image() ) {
//                    target_buf->get_image().splat( img_buf->get_image(), el.smooth, el.position, el.scale, th, mask, tint, el.mmode ); 
                    target_buf->get_image().splat( img_buf->get_image(), el.smooth, el.position, el.scale, th, mask, tint, el.mmode ); 
                }
            }
            else std::cout << "splat_element() - no image in buffer" << std::endl;
        }
        else std::cout << "splat_element() - null image buffer" << std::endl;
    }
    else std::cout << "splat_element() - unmatched image buffer" << std::endl;
}
element_context::element_context( scene& s_init, any_buffer_pair_ptr& buf_init ) : el(default_element), cl(default_cluster), s(s_init), buf(buf_init) {}

// splats any element onto any image
void element::render( any_buffer_pair_ptr& target ) { 
    //std::cout << " element::render\n";
    pixel_type ptype = ( pixel_type )target.index();
    //std::cout << " pixel type " << ptype << std::endl;
    switch( ( pixel_type )target.index() ) {     // replace with std::visit
        case( PIXEL_FRGB   ): splat_element< frgb   >( std::get< std::shared_ptr< buffer_pair< frgb > > >( target ), *this ); break;
        case( PIXEL_UCOLOR ): splat_element< ucolor >( std::get< std::shared_ptr< buffer_pair< ucolor > > >( target ), *this ); break;
        case( PIXEL_VEC2F  ): splat_element< vec2f  >( std::get< std::shared_ptr< buffer_pair< vec2f > > >( target ), *this ); break;
        case( PIXEL_INT    ): splat_element< int    >( std::get< std::shared_ptr< buffer_pair< int > > >( target ), *this ); break;
        case( PIXEL_VEC2I  ): splat_element< vec2i  >( std::get< std::shared_ptr< buffer_pair< vec2i > > >( target ), *this ); break;
    }
}

void element::operator () ( any_buffer_pair_ptr& buf, element_context& context ) {
    render( buf );
}

// Recursively generate and render elements
void cluster::render( scene& s, any_buffer_pair_ptr& buf ) { 
    element el = root_elem;
    element_context context( el, *this, s, buf );
    max_n( context ); min_scale( context ); max_depth( context ); // set cluster harnesses
    while( next_elem( context ) ) el.render( buf ); 
    
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

void effect_list::resize( vec2i new_dim ) {
    //if( new_dim == *dim ) return;
    // need to resize buffer (retaining data) instead?
    std::cout << "effect_list " << name << " resize() " << new_dim.x << " " << new_dim.y << std::endl;
    std::visit( [&]( auto& b ) { b->reset( new_dim ); }, buf );
    dim = new_dim;       
}

void effect_list::update( scene& s ) {
    // create dummy context
    element_context context( s, buf );
    /*if( name == "Self") {
        any_buffer_pair_ptr buf = context.s.buffers[ name ];
        vec2i val;
        std::visit( [&]( auto& b ) { val = b->get_image().get_dim(); }, buf );
        std::cout << "effect_list " << name << " update() - buffer dim " << val.x << " " << val.y << std::endl;
    }*/

    if( self_generated ) {
        vec2i old_dim = *dim;
        dim( context );
        vec2i new_dim = *dim;
        //if( name == "warp_vf" ) std::cout << "effect_list " << name << " update() - new dim " << new_dim.x << " " << new_dim.y << std::endl;
        if( new_dim != old_dim ) {
            resize( new_dim );
            rendered = false;
            if( name == "Self" ) s.self_dim = new_dim;
        }
    }
    else {
        // check for new source buffer
        std::string old_name = *source_name; 
        source_name(context);

        if (*source_name != old_name) {
            rendered = false;
        }

        if (rmode == MODE_EPHEMERAL || !rendered) {

            if (s.buffers.contains(*source_name)) {
                any_buffer_pair_ptr& source_buf = s.buffers[*source_name];
                vec2i source_dim;

                std::visit([&](auto& b) {
                    source_dim = b->get_image().get_dim();
                }, source_buf);

                if (source_dim != *dim) {
                    dim = source_dim;
                    resize(source_dim);
                    if( name == "Self" ) s.self_dim = source_dim;
                }

                // Copying here may be an unnecessary extra step - can we render directly from source buffer?
                std::visit([&, source_buf](auto& b) {
                    //std :: cout << "effect_list " << name << " update() - copying source buffer " << *source_name << std::endl;
                    copy_buffer(b, source_buf);
                }, buf);
            } else {
                std::cout << "effect_list::update() - what happens here?" << std::endl;
            }
        }
    }
}

/*
void effect_list::update_source_name( scene& s ) {
    //std::cout << "effect_list " << name << " update_source_name() ";
    // create dummy context
    element el;
    next_element ne;
    cluster cl( el, ne );
    element_context context( el, cl, s, buf );

    // check for new source buffer
    std::string old_name = *source_name; 
    source_name( context );
    //std::cout << " old source name: " << old_name << " new source name: " << *source_name << std::endl;
    if( *source_name != old_name ) rendered = false;
}
 
// Could be changed to allow different size than source buffer
void effect_list::copy_source_buffer( scene& s ) {
    // get dimensions of source buffer - resize output buffer if necessary
    if( rmode == MODE_EPHEMERAL || !rendered ) { // ephemeral buffers are re-rendered each frame
        //std::cout << "preparing to copy buffer " << *source_name << std::endl;
        if( s.buffers.contains( *source_name ) ) {  // load source image or result into buffer
            // Copy source buffer into working buffer 
            any_buffer_pair_ptr& source_buf = s.buffers[ *source_name ];
            vec2i source_dim;
            std::visit( [&]( auto& b ) { source_dim = b->get_image().get_dim(); }, source_buf );
            if( source_dim != *dim ) {
                dim = source_dim;
                resize( *dim );
            }
            if( !self_generated ) {
                //std::cout << "effect_list" << name << " copy_source_buffer() - copying buffer " << *source_name << std::endl;
                std::visit( [ &, source_buf ]( auto& b ) { copy_buffer( b, source_buf ); }, buf );
            }
        }
        else {  // blank buffer
            //throw std::runtime_error( "effect_list::render() - source buffer not found" );
            //std::cout << "effect_list::copy_source_buffer() - source buffer not found" << std::endl;
            //resize( dim );
        }
    }
}
*/
void effect_list::render( scene& s ) {
    // todo: what happens if effect list is targeting a buffer that depends on itself?

    update( s );
    if( !( rmode == MODE_STATIC && rendered ) ) { // no need to re-render static buffers
        for( auto& e : effects ) {
            // default element and cluster
            element default_element;
            next_element default_next_element;
            cluster default_cluster( default_element, default_next_element );
            element_context context( default_element, default_cluster, s, buf );
            //std::cout << "about to render effect " << e << " into buffer " << name << std::endl;
            s.effects[ e ]( buf, context );
            //std::cout << "rendered effect " << e << " into buffer " << name << std::endl;
        }
        rendered = true;
        // use std::visit to call mip_it on image pixels
        //std::visit( [&]( auto& b ) { b->get_image().use_mip(true); b->get_image().mip_it(); }, buf );
    }
    //else std::cout << "static or already rendered" << std::endl;
    // std::cout << "effect_list render complete" << std::endl;
}

void effect_list::restart( scene& s ) {
    //std::cout << "effect_list " << name << " restart()" << std::endl;
    rendered = false;
    update( s );
}

scene::scene( float time_interval_init ) : liveCamera( false ), time_interval( time_interval_init ), default_time_interval( time_interval_init ) {}

scene::scene( const std::string& filename, float time_interval_init ) 
    : liveCamera( false ), time( 0.0f ), time_interval( time_interval_init ), default_time_interval( time_interval_init )
{
    scene_reader reader( *this, filename );
}

vec2f scene::get_mouse_pos() const {
    bb2f bounds = std::visit( [&]( auto& b ) { return b->get_image().get_bounds(); }, queue.back().buf );
    bounds.print();
    ui.canvas_bounds.print();
    return bounds.bb_map( ui.mouse_pixel, ui.canvas_bounds );
}

void scene::restart() {
    time = 0.0f;
    time_interval = default_time_interval;
    for( auto& eff_list : queue ) eff_list.restart( *this );
}

void scene::set_time_interval( const float& t ) {
    time_interval = t;
}

void scene::render() {
    for( auto& eff_list : queue ) eff_list.render( *this );
    time += time_interval;
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
    //std::cout << "scene::render_and_save() dim = " << dim.x << " " << dim.y << std::endl;
    any_buffer_pair_ptr any_out;
    // bounds set automatically by image constructor
    switch( ptype ) {
        case( PIXEL_FRGB   ): any_out = std::make_shared< buffer_pair< frgb >   >( dim ); break;
        case( PIXEL_UCOLOR ): any_out = std::make_shared< buffer_pair< ucolor > >( dim ); break;
        case( PIXEL_VEC2F  ): any_out = std::make_shared< buffer_pair< vec2f >  >( dim ); break;
        case( PIXEL_INT    ): any_out = std::make_shared< buffer_pair< int >    >( dim ); break;
        case( PIXEL_VEC2I  ): any_out = std::make_shared< buffer_pair< vec2i >  >( dim ); break;
    }
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
    //std::cout << "scene::animate() dim = " << dim.x << " " << dim.y << std::endl;

    any_buffer_pair_ptr any_out;
    switch( ptype ) {
        case( PIXEL_FRGB   ): any_out = std::make_shared< buffer_pair< frgb >   >( dim ); break;
        case( PIXEL_UCOLOR ): any_out = std::make_shared< buffer_pair< ucolor > >( dim ); break;
        case( PIXEL_VEC2F  ): any_out = std::make_shared< buffer_pair< vec2f >  >( dim ); break;
        case( PIXEL_INT    ): any_out = std::make_shared< buffer_pair< int >    >( dim ); break;
        case( PIXEL_VEC2I  ): any_out = std::make_shared< buffer_pair< vec2i >  >( dim ); break;
    }
    set_output_buffer( any_out ); // set output buffer

    time = 0.0f;
    for( int frame = 0; frame < nframes; frame++ ) {
        std::ostringstream s;
        s << basename << std::setfill('0') << std::setw(4) << frame << ".jpg";
        std::string filename = s.str();
        render();
        save_result( filename, dim, ptype, ftype, quality );
        std::cout << "frame " << frame << " time " << time << std::endl;
    }

    // future: make the video file here 
}

void scene::set_output_buffer( any_buffer_pair_ptr& buf ) {
    auto& output_list = queue.back();
    output_list.buf = buf;
    std::visit( [&]( auto& b ) { b->reset( *(output_list.dim) ); }, buf );
    output_list.ptype = ( pixel_type )buf.index();
    vec2i dim_out;
    std::visit( [&]( auto& b ) { dim_out = b->get_image().get_dim(); }, buf );
    std::cout << "scene::set_output_buffer() dim_out " << dim_out.x << " " << dim_out.y << std::endl << std::endl;
    self_dim = dim_out;

    for( int i = 0; i < queue.size() - 1; i++ ) {
        auto& eff_list = queue[ i ];
        eff_list.rendered = false;
        vec2i dim = { static_cast<int>(std::round( dim_out.x * eff_list.relative_dim )), static_cast<int>(std::round( dim_out.y * eff_list.relative_dim )) };
        eff_list.resize( dim );
    }
}

effect_list& scene::get_effect_list( const std::string& name ) {
    for( auto& eff_list : queue )
        if( eff_list.name == name )
            return eff_list;
    throw std::runtime_error( "scene::get_effect_list: effect list" + name + " not found" );
} 