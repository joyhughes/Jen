#include "next_element.hpp"

template< class T > bool advect_element< T >::operator () ( element_context< T >& context, const float& t ) { 
    element< T >& el = context.el;
    if( proportional ) el.position = vf.advect( el.position, step * el.scale, el.orientation, smooth, SAMP_REPEAT ); 
    else               el.position = vf.advect( el.position, step,            el.orientation, smooth, SAMP_REPEAT ); 
    return true;
}

template class advect_element< frgb >;       
template class advect_element< ucolor >;       
template class advect_element< vec2f >;   

template< class T > void angle_branch< T >::render_branch( const float& ang, element_context< T >& context, const float& t )
{
    cluster< T > cl( context.cl ); 
    cl.set_root( context.el );
    element< T >& el = cl.root_elem;
    cl.depth++;
    el.scale *= size_prop;
    if( ( el.scale >= min_scale ) && ( cl.depth <= cl.max_depth ) ) {
        // change branch rule or other cluster params here
        el.orientation += ang;
        el.derivative = rot_deg( el.derivative, ang );
        el.index = 0;
        // calculate new position - relative to angle of motion if relevant
        //float ang = vtoa( el.derivative ) + branch_ang;
        el.position += linalg::normalize( el.derivative ) * ( context.el.scale + el.scale );
        // render branch
        cl.render( context.img );
    }
}

template< class T > bool angle_branch< T >::operator () ( element_context< T >& context, const float& t ) {
    element< T >& el = context.el;
    if( !( ( el.index + offset ) % interval ) ) render_branch( branch_ang, context, t );
    if( mirror_offset.has_value() ) {
        if( !( ( el.index + *mirror_offset ) % interval ) ) render_branch( -branch_ang, context, t );
    }
    return true;
}

/*
template< class T > bool angle_branch< T >::operator () ( element_context< T >& context, const float& t ) {
    element< T >& el = context.el;
    cluster< T >& cl = context.cl;
    if( !( ( el.index + offset ) % interval ) ) {
        branch.depth = cl.depth + 1;
        branch.root_elem.scale = el.scale * size_prop;
        std::cout << "depth " << cl.depth << " \n";
        if( ( branch.root_elem.scale >= min_scale ) && ( branch.depth <= branch.max_depth ) ) {
            // change branch rule or other cluster params here
            branch.root_elem.orientation += branch_ang;
            branch.root_elem.index = 0;
            // calculate new position - relative to angle of motion if relevant
            float ang = vtoa( el.derivative ) + branch_ang;
            cl.root_elem.position += unit_vector( ang ) * ( el.scale + cl.root_elem.scale );
            // render branch
            cl.render( context.img );
        }
    }
    return true;
}
*/
template class angle_branch< frgb >;       
template class angle_branch< ucolor >;       
template class angle_branch< vec2f >;   

template< class T > bool next_element< T >::operator () ( element_context< T >& context, const float& t ) { 
    element< T >& el = context.el;
    el.index++;
    if( el.index >= max_index ) return false;
    vec2f p = el.position;
    for( auto fn : functions ) if( !fn( context, t ) ) return false;
    el.derivative = el.position - p;
    // bounds check
    if( bounds.has_value() ) if( !( bounds->in_bounds( el.position ) ) ) return false;
    return true; 
}

template class next_element< frgb >;       
template class next_element< ucolor >;       
template class next_element< vec2f >;       
