
#include "effect.hpp"
#include "scene.hpp"
#include "life.hpp"

void eff_identity::operator () ( any_buffer_pair_ptr &buf, element_context& context ) {} // Does nothing

// Fill image with color
template< class T > void eff_fill< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    //std::cout << "eff_fill: " << std::endl;
    fill_color( context ); 
    bounds( context );
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_fill: no image in buffer" );
        if( bounded ) buf_ptr->get_image().fill( *fill_color, *bounds );
        else          buf_ptr->get_image().fill( *fill_color );
    }
    // else convert between pixel types
}

template class eff_fill< frgb >;
template class eff_fill< ucolor >;
template class eff_fill< vec2f >;
template class eff_fill< int >;
template class eff_fill< vec2i >;

// Noise effect
template< class T > void eff_noise< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    bounds( context ); 
    a( context );
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_fill: no image in buffer" );
        if( bounded ) buf_ptr->get_image().noise( *a, *bounds );
        else          buf_ptr->get_image().noise( *a );
    }
    // else convert between pixel types
}

template class eff_noise< frgb >;
template class eff_noise< ucolor >;
template class eff_noise< vec2f >;
template class eff_noise< int >;
template class eff_noise< vec2i >;

template< class T > void eff_checkerboard< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    c1( context ); c2( context ); box_size( context );
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_checkerboard: no image in buffer" );
        buf_ptr->get_image().checkerboard( *box_size, *c1, *c2 );
    }
}

template class eff_checkerboard< frgb >;
template class eff_checkerboard< ucolor >;
template class eff_checkerboard< vec2f >;
template class eff_checkerboard< int >;
template class eff_checkerboard< vec2i >;

template< class T > void eff_grayscale< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_grayscale: no image in buffer" );
        buf_ptr->get_image().grayscale();
    }
}

template class eff_grayscale< frgb >;
template class eff_grayscale< ucolor >;

template< class T > void eff_invert< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    get_image< T >( buf ).invert();
}

template class eff_invert< frgb >;
template class eff_invert< ucolor >;

template< class T > void eff_rotate_components< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_grayscale: no image in buffer" );
        buf_ptr->get_image().rotate_components( *r );
    }
}

template class eff_rotate_components< frgb >;
template class eff_rotate_components< ucolor >;

template< class T > void eff_rgb_to_hsv< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_rgb_to_hsv: no image in buffer" );
        buf_ptr->get_image().image<T>::rgb_to_hsv();
        //auto img = buf_ptr->get_image();
        //for( auto c = img.begin(); c != img.end(); c++ ) *c = rgb_to_hsv( *c );
    }
}

template class eff_rgb_to_hsv< frgb >;
template class eff_rgb_to_hsv< ucolor >;

template< class T > void eff_hsv_to_rgb< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_hsv_to_rgb: no image in buffer" );
        buf_ptr->get_image().hsv_to_rgb();
    }
}

template class eff_hsv_to_rgb< frgb >;
template class eff_hsv_to_rgb< ucolor >;

template< class T > void eff_rotate_hue< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
//    float old_offset = *offset;
    offset( context );
//    if( *offset != old_offset ) {
        if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
        {
            auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
            if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_rotate_hue: no image in buffer" );
            buf_ptr->get_image().rotate_hue( *offset );
        }
//    }
}

template class eff_rotate_hue< frgb >;
template class eff_rotate_hue< ucolor >;

template< class T > void eff_posterize< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    h_levels( context ); s_levels( context ); v_levels( context );
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_posterize: no image in buffer" );
        buf_ptr->get_image().posterize( *h_levels, *s_levels, *v_levels );
    }
}

template class eff_posterize< ucolor >;

template< class T > void eff_bit_plane< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_bit_plane: no image in buffer" );
        bit_mask( context );
        buf_ptr->get_image().bit_plane( *bit_mask );
    }
}

template class eff_bit_plane< ucolor >;

template< class T > void eff_crop_circle< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_crop_circle: no image in buffer" );
        background( context ); ramp_width( context );
        buf_ptr->get_image().crop_circle( *background, *ramp_width );
    }
}

template class eff_crop_circle< frgb >;
template class eff_crop_circle< ucolor >;
template class eff_crop_circle< vec2f >;
template class eff_crop_circle< int >;
template class eff_crop_circle< vec2i >;

template< class T > void eff_mirror< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_mirror: no image in buffer" );
        buf_ptr->get_buffer().mirror( buf_ptr->get_image(), reflect_x, reflect_y, top_to_bottom, left_to_right, *center, extend );
        buf_ptr->swap();
    }
}

template class eff_mirror< frgb >;
template class eff_mirror< ucolor >;
template class eff_mirror< vec2f >;
template class eff_mirror< int >;
template class eff_mirror< vec2i >;

template< class T > void eff_turn< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_turn: no image in buffer" );
        buf_ptr->get_buffer().turn( buf_ptr->get_image(), direction );
        buf_ptr->swap();
    }
}

template class eff_turn< frgb >;
template class eff_turn< ucolor >;
template class eff_turn< vec2f >;
template class eff_turn< int >;
template class eff_turn< vec2i >;

template< class T > void eff_flip< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_flip: no image in buffer" );
        buf_ptr->get_buffer().flip( buf_ptr->get_image(), flip_x, flip_y );
        buf_ptr->swap();
    }
} 

template class eff_flip< frgb >;
template class eff_flip< ucolor >;
template class eff_flip< vec2f >;
template class eff_flip< int >;
template class eff_flip< vec2i >;

template< class T > void eff_vector_warp< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )  { 
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        vf_name( context ); step( context ); smooth( context ); relative( context ); extend( context );
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_vector_warp: no image in buffer" );
        auto &vf = context.s.get_image< vec2f >( *vf_name );
        buf_ptr->get_buffer().warp( buf_ptr->get_image(), vf, *step, *smooth, *relative, *extend );
        buf_ptr->swap();
    }
}

template class eff_vector_warp< frgb >;
template class eff_vector_warp< ucolor >;
template class eff_vector_warp< vec2f >;
template class eff_vector_warp< int >;
template class eff_vector_warp< vec2i >;

template< class T > void eff_feedback< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    //std::cout << "eff_feedback" << std::endl;
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf))
    {
        wf_name( context );
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_feedback: no image in buffer" );
        auto& wf = context.s.get_image< int >( *wf_name );
        buf_ptr->get_buffer().warp( buf_ptr->get_image(), wf );
        buf_ptr->swap();
    }
}

template class eff_feedback< frgb >;
template class eff_feedback< ucolor >;
template class eff_feedback< vec2f >;
template class eff_feedback< int >;
template class eff_feedback< vec2i >;

// Vector field effects

template< class T > void eff_complement< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    vf_tools tools( get_image< T >( buf ) );
    tools.complement();
}

template class eff_complement< vec2f >;

template< class T > void eff_radial< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    vf_tools tools( get_image< T >( buf ) );
    tools.radial();
}

template class eff_radial< vec2f >;

template< class T > void eff_cartesian< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    vf_tools tools( get_image< T >( buf ) );
    tools.cartesian();
}

template class eff_cartesian< vec2f >;

template< class T > void eff_rotate_vectors< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    vf_tools tools( get_image< T >( buf ) );
    tools.rotate_vectors( *angle );
}

template class eff_rotate_vectors< vec2f >;

template< class T > void eff_scale_vectors< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    scale( context );
    get_image< T >( buf ) *= *scale;
}

template class eff_scale_vectors< vec2f >;

template< class T > void eff_normalize< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    vf_tools tools( get_image< T >( buf ) );
    tools.normalize();
}

template class eff_normalize< vec2f >;

template< class T > void eff_inverse< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    diameter( context ); soften( context );
    vf_tools tools( get_image< T >( buf ) );
    tools.inverse( *diameter, *soften );
}

template class eff_inverse< vec2f >;

template< class T > void eff_inverse_square< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    diameter( context ); soften( context );
    vf_tools tools( get_image< T >( buf ) );
    tools.inverse_square( *diameter, *soften );
}

template class eff_inverse_square< vec2f >;

template< class T > void eff_concentric< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    center( context );
    vf_tools tools( get_image< T >( buf ) );
    tools.concentric( *center );
}

template class eff_concentric< vec2f >;

template< class T > void eff_rotational< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    if (std::holds_alternative< vbuf_ptr>(buf)) 
    {
        auto& buf_ptr = std::get< vbuf_ptr >(buf);
        vf_tools tools( buf_ptr->get_image() );
        tools.rotation( *center );
    }
}

template class eff_rotational< vec2f >;

template< class T > void eff_spiral< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    if (std::holds_alternative< vbuf_ptr>(buf)) 
    {
        auto& buf_ptr = std::get< vbuf_ptr >(buf);
        vf_tools tools( buf_ptr->get_image() );
        tools.concentric( *center );
        tools.rotate_vectors( *angle );
    }
}

template class eff_spiral< vec2f >;

template< class T > void eff_fermat_spiral< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    c(context);
    vf_tools tools( get_image< T >( buf ) );
    tools.fermat_spiral( *c );
}

template class eff_fermat_spiral< vec2f >;

template< class T > void eff_vortex< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    if (std::holds_alternative< vbuf_ptr>(buf)) 
    {
        auto& buf_ptr = std::get< vbuf_ptr >(buf);
        vf_tools tools( buf_ptr->get_image() );
        vortex v( *diameter, *soften, *intensity, *center_orig, revolving, *velocity, *center_of_revolution );
        tools.vortex( v );
    }
}

template class eff_vortex< vec2f >;

template< class T > void eff_turbulent< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    if (std::holds_alternative< vbuf_ptr>(buf)) 
    {
        auto& buf_ptr = std::get< vbuf_ptr >(buf);
        vf_tools tools( buf_ptr->get_image() );
        vortex_field v( *n, revolving, *scale_factor, *min_diameter, *max_diameter, *min_soften, *max_soften, *min_intensity, *max_intensity, intensity_direction, *min_velocity, *max_velocity, velocity_direction, *min_orbital_radius, *max_orbital_radius );
        tools.turbulent( v );
    }
}

template class eff_turbulent< vec2f >;

template< class T > void eff_position_fill< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    if (std::holds_alternative< std::shared_ptr< buffer_pair< T > > >(buf)) 
    {
        auto& buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >(buf);
        if( !buf_ptr->has_image() ) throw std::runtime_error( "eff_position_fill: no image in buffer" );
        vf_tools tools( buf_ptr->get_image() );
        tools.position_fill();
    }
}

template class eff_position_fill< vec2f >;

template< class T > void eff_kaleidoscope< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    //vec2f old_center=*center; float old_segments=*segments; float old_start = *start; float old_spin = *spin; bool old_reflect=*reflect;
    segments( context ); levels( context );
    start( context ); spin( context );
    level_start( context ); expand( context );
    reflect( context ); reflect_levels( context );
    
    filled =true;
    vf_tools tools( get_image< T >( buf ) );
    tools.kaleidoscope( *segments, *levels, *start, *spin, *level_start, *expand, *reflect, *reflect_levels );  
}

template class eff_kaleidoscope< vec2f >;

template< class T > void eff_radial_tile< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    segments(context);  levels(context); 
    offset_x(context);  offset_y(context); 
    spin( context );    expand( context );
    zoom_x(context);    zoom_y(context); 
    reflect_x(context); reflect_y(context);
    
    vf_tools tools( get_image< T >( buf ) );
    tools.radial_tile( *segments, *levels, vec2f( *offset_x, *offset_y ), *spin, *expand, vec2f( *zoom_x, *zoom_y ), *reflect_x, *reflect_y );
}

template class eff_radial_tile< vec2f >;

template< class T > void eff_radial_multiply< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    segments(context);  levels(context);  
    spin( context );  expand( context );
    reflect( context ); reflect_levels( context );

    if (std::holds_alternative< vbuf_ptr>(buf)) 
    {
        auto& buf_ptr = std::get< vbuf_ptr >(buf);
        vf_tools tools( buf_ptr->get_image() );
        tools.radial_multiply( *segments, *levels, *spin, *expand, *reflect, *reflect_levels );
    }
}

template class eff_radial_multiply< vec2f >;

template< class T > void eff_theta_swirl< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    amount( context );

    if (std::holds_alternative< vbuf_ptr>(buf)) 
    {
        auto& buf_ptr = std::get< vbuf_ptr >(buf);
        vf_tools tools( buf_ptr->get_image() );
        tools.theta_swirl( *amount );
    }
}

template class eff_theta_swirl< vec2f >;

template< class T > void eff_theta_rotate< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    angle( context );

    if (std::holds_alternative< vbuf_ptr>(buf)) 
    {
        auto& buf_ptr = std::get< vbuf_ptr >(buf);
        vf_tools tools( buf_ptr->get_image() );
        tools.theta_rotate( *angle );
    }
}

template class eff_theta_rotate< vec2f >;

template< class T > void eff_theta_rings< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    n( context ); swirl( context ); alternate( context );

    if (std::holds_alternative< vbuf_ptr>(buf)) 
    {
        auto& buf_ptr = std::get< vbuf_ptr >(buf);
        vf_tools tools( buf_ptr->get_image() );
        tools.theta_rings( *n, *swirl, *alternate );
    }
}

template class eff_theta_rings< vec2f >;

template< class T > void eff_theta_waves< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    freq( context ); amp( context ); phase( context ); const_amp( context );

    if (std::holds_alternative< vbuf_ptr>(buf)) 
    {
        auto& buf_ptr = std::get< vbuf_ptr >(buf);
        vf_tools tools( buf_ptr->get_image() );
        tools.theta_waves( *freq, *amp, *phase, *const_amp );
    }
}

template class eff_theta_waves< vec2f >;

template< class T > void eff_theta_saw< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    freq( context ); amp( context ); phase( context ); const_amp( context );

    if (std::holds_alternative< vbuf_ptr>(buf)) 
    {
        auto& buf_ptr = std::get< vbuf_ptr >(buf);
        vf_tools tools( buf_ptr->get_image() );
        tools.theta_saw( *freq, *amp, *phase, *const_amp );
    }
}

template class eff_theta_saw< vec2f >;

template< class T > void eff_theta_compression_waves< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    freq( context ); amp( context ); phase( context ); const_amp( context );

    if (std::holds_alternative< vbuf_ptr>(buf)) 
    {
        auto& buf_ptr = std::get< vbuf_ptr >(buf);
        vf_tools tools( buf_ptr->get_image() );
        tools.theta_compression_waves( *freq, *amp, *phase, *const_amp );
    }
}

template class eff_theta_compression_waves< vec2f >;

template< class T > void eff_fill_warp< T >::operator () ( any_buffer_pair_ptr& buf, element_context& context )
{
    //std::cout << "eff_fill_warp" << std::endl;
    vf_name( context ); relative( context ); extend( context );
    if( *vf_name != "none" && *vf_name != "" ) {
        image< vec2f >& vf = context.s.get_image< vec2f >( *vf_name );
        get_image< T >( buf ).fill( vf, *relative, *extend );
    }
}

template class eff_fill_warp< int >;

void eff_n::operator () ( any_buffer_pair_ptr &buf, element_context& context )
{
    n( context );
    for( int i = 0; i < *n; i++ ) eff( buf, context );
}

void eff_n::set_effect( any_effect_fn& eff_init ) { eff = eff_init; }

//void eff_n( int& n_init, any_effect_fn& eff_init ) : n( n_init ), eff( eff_init ) {}

// Composite effect - runs a list of component effects
void eff_composite::operator () ( any_buffer_pair_ptr& buf, element_context& context )  {
    for( auto eff : effects ) { eff( buf, context );  }
}

void eff_composite::add_effect( const any_effect_fn& eff ) { effects.push_back( eff ); }

void eff_chooser::operator () ( any_buffer_pair_ptr& buf, element_context& context )  {
    choice( context );
    if( ( *choice < effects.size() ) && ( *choice >= 0 ) ) effects[ *choice ]( buf, context );
}

void eff_chooser::choose( int my_choice ) { 
    if( ( my_choice < effects.size() ) && ( my_choice >= 0 ) ) choice = my_choice; 
}

void eff_chooser::choose( const std::string& name ) { 
    for( int i = 0; i < effects.size(); i++ ) {
        if( effects[ i ].name == name ) choice = i;
    }
}

void eff_chooser::add_effect( const any_effect_fn& eff ) { effects.push_back( eff ); }

