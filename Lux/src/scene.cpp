#include "scene.hpp"
#include "next_element.hpp"
//#include "any_image.hpp"
#include "fimage.hpp"
#include "uimage.hpp"
#include "vector_field.hpp"
#include <optional>

// splats any element onto a particular image
template< class T > void splat_element( image< T >& target, element& el, const pixel_type& pix_type ) {
    typedef std::optional< std::reference_wrapper< T > > opt_pix_ref;
    typedef std::optional< std::reference_wrapper< image< T > > > opt_img_ref;

    resolve< std::reference_wrapper< image< T > > > image_resolver;
    resolve< std::reference_wrapper< T > >       pixel_resolver;
    auto img_resolved =  std::visit( image_resolver, el.img );
    auto mask_resolved = std::visit( image_resolver, el.mask );
    auto tint_resolved = std::visit( pixel_resolver, el.tint );

    float th = el.rotation;
    if( el.orientation_lock ) th += el.orientation;

    target.splat( el.position, el.scale, th, img_resolved, mask_resolved, tint_resolved, el.mmode ); 
}

// splats any element onto any image
void element::render( any_image target, const float& t ) { 
        // todo: run (potentially time-dependent) effect
        //std::visit( [ this, r ]( auto&& arg ){ arg.get().splat( this->position, this->scale, r, this->img, this->mask, this->tint ); }, in );
        pixel_type pix_type = ( pixel_type )target.index();
        //any_splat my_splat( *this, pix_type );
        //std::visit( my_splat, target );
        std::visit(overloaded {
            [ this, pix_type ]( std::monostate arg )   {},
            [ this, pix_type ]( image< frgb   >& arg ) { splat_element< frgb   >( arg, *this, pix_type ); },
            [ this, pix_type ]( image< ucolor >& arg ) { splat_element< ucolor >( arg, *this, pix_type ); },
            [ this, pix_type ]( image< vec2f  >& arg ) { splat_element< vec2f  >( arg, *this, pix_type ); }
        }, target );
}

/*
void element::operator () ( any_buffer_pair buf, const float& t ) {
    render( buf.get_image(), t );
    std::visit( [ this ]( auto&& arg ){ this->render( arg.get().get_image(), t ); }, buf );
}
*/

// Recursively generate and render elements
void cluster::render( any_image img, const float& t ) { 
    element el = root_elem;
    element_context context( el, *this, img, t );
    el.render( img, t );
    while( next_elem( context ) ) { el.render( img, t ); }
    //( ( fimage & )img ).write_jpg( "hk_cluster.jpg", 100 ); // debug - save frame after each cluster                   
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