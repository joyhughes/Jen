#include "next_element.hpp"
#include "any_function.hpp"
#include "scene.hpp"
#include "life.hpp"

template< class U > void harness< U >::operator () ( element_context &context ) 
{ for( auto& fn : functions ) {
    //std::cout << "      harness function " << fn.name << std::endl;
    val = fn( val, context ); 
    }
}

template< class U > void harness< U >::add_function( const any_fn< U >& fn)
{ functions.push_back( fn ); }

template< class U > harness< U >::harness() {}

template< class U > harness< U >::harness( const U& val_init ) : val( val_init ) {}

template< class U > inline harness< U >::harness(const harness< U > &h) {
    val = h.val;
    for (int i = 0; i < h.functions.size(); i++) 
        functions.push_back( h.functions[ i ] ); 
}

template< class U > harness< U >::~harness() {}

template struct harness< float >;
template struct harness< vec2f >;
template struct harness< int >;
template struct harness< vec2i >;
template struct harness< frgb >;
template struct harness< ucolor >;
template struct harness< bb2f >;
template struct harness< std::string >;
template struct harness< bool >;
template struct harness< direction4 >; // future: replace enum harnesses with int harnesses
template struct harness< direction8 >; // future: replace enum harnesses with int harnesses
template struct harness< interval_int >;
template struct harness< interval_float >;
template struct harness< box_blur_type >; // future: replace enum harnesses with int harnesses
//template struct harness< std::vector< int > >;

/*
template< class U > U identity_fn< U >::operator () ( U& in, element_context& context ) { 
    return in; 
}
*/

float time_fn::operator () ( float& val, element_context& context  ) { 
    return context.s.time; 
}

float wiggle::operator ()  ( float& val, element_context& context  )
{
    wavelength( context ); amplitude( context ); phase( context ); wiggliness( context );
    //std::cout << "wiggle: val " << val  << " wavelength " << *wavelength << " amplitude " << *amplitude << " phase " << *phase << " wiggliness " << *wiggliness << std::endl;
    if( *wavelength != 0.0f ) {
        //std::cout << "   wiggle return value " << *amplitude * sin( ( val / *wavelength + *phase + *wiggliness * context.t ) * TAU ) << std::endl;
        return *amplitude * sin( ( val / *wavelength + *phase + *wiggliness * context.s.time ) * TAU );
    }
    else return 0.0f; 
}

vec2f mouse_pos_fn::operator () ( vec2f& val, element_context& context  )
{
    return context.s.get_mouse_pos();
}

vec2i mouse_pix_fn::operator () ( vec2i& val, element_context& context  )
{
    return context.s.ui.mouse_pixel;
}

template< class U > U index_param< U >::operator () ( U &val, element_context &context ) { 
    U arg = context.el.index * iden< U >();
    return fn( arg, context );
}

template< class U > index_param< U >::index_param() {}
template< class U > index_param< U >::index_param( any_fn<U> &fn_init ) { fn = fn_init; }

template struct index_param< float >;

template< class U > U scale_param< U >::operator () ( U &val, element_context &context ) { 
    U arg = context.el.scale * iden< U >();
    return fn( arg, context );
}

template< class U > scale_param< U >::scale_param() {}
template< class U > scale_param< U >::scale_param( any_fn<U> &fn_init ) { fn = fn_init; }

template struct scale_param< float >;

template< class U > U time_param< U >::operator () ( U &val, element_context &context ) { 
    U arg = context.s.time * iden< U >();
    return fn( arg, context );
}

/*
template<> float time_param< float >::operator () ( float &val, element_context &context ) { 
    return fn( context.t, context );
}
*/

template< class U > time_param< U >::time_param() {}
template< class U > time_param< U >::time_param( any_fn<U> &fn_init ) { fn = fn_init; }

template struct time_param<  float >;

void orientation_gen_fn::operator () ( element_context& context ) { 
    orientation.val = context.el.orientation;
    orientation( context );
    context.el.orientation = *orientation;
}

void scale_gen_fn::operator () ( element_context& context ) { 
    scale.val = context.el.scale;
    scale( context );
    context.el.scale = *scale; 
}

void rotation_gen_fn::operator () ( element_context& context ) { 
    r.val = context.el.rotation;
    r( context );
    context.el.rotation = *r; 
}

void position_gen_fn::operator () ( element_context& context ) { 
    position.val = context.el.position;
    position( context );
    context.el.position = *position; 
}

void advect_element::operator () ( element_context& context ) { 
    flow( context ); step( context );
    element& el = context.el;
    vec2f dir = *flow;
    if( orientation_sensitive ) dir = linalg::rot( el.orientation / 360.0f * TAU, dir );
    
    float prop = *step;
    if( proportional ) prop *= el.scale; 
    if( time_interval_proportional ) prop *= context.s.time_interval;
    el.position += dir * prop;
}

void angle_branch::render_branch( const float& ang, element_context& context )
{
    cluster cl( context.cl ); 
    cl.set_root( context.el );
    cl.max_depth( context );
    element& el = cl.root_elem;
    cl.depth++;
    el.scale *= *size_prop;
    if( cl.depth <= *cl.max_depth ) {
        // change branch rule or other cluster params here
        el.orientation += ang;
        el.derivative = rot_deg( el.derivative, ang );
        el.index = 0;
        // calculate new position - relative to angle of motion if relevant
        //float ang = vtoa( el.derivative ) + branch_ang;
        el.position += *branch_dist * ( linalg::normalize( el.derivative ) * ( context.el.scale + el.scale ) );
        // render branch (depth first traversal)
        cl.render( context.s, context.buf );
    }
}

void angle_branch::operator () ( element_context& context ) {
    size_prop( context ); branch_ang( context ); branch_dist( context );
    element& el = context.el;
    if( !( ( el.index + offset ) % interval ) ) render_branch( *branch_ang, context );
    if( mirror_offset.has_value() ) {
        if( !( ( el.index + *mirror_offset ) % interval ) ) render_branch( -*branch_ang, context );
    }
}

void curly::operator () ( element_context& context ) {
    auto c = *curliness;
    curliness( context );
    element& el = context.el;
    if( el.scale > 0.0f ) el.orientation += *curliness / el.scale;
    curliness = c;  // forget
}

/*
bool position_list::operator () ( element_context& context ) {
    positions( context );
    element& el = context.el;
    if( el.index < (*positions).size() ) el.position = (*positions)[ el.index ];
    return true;
}
*/

// Conditions for index_filter
bool initial_element_condition  ::operator () ( element_context& context ) { 
    //std::cout << "initial_element_condition" << std::endl;
    return  ( context.el.index == 0 ); 
}

bool initial_element_fn        ::operator () ( bool& val, element_context& context ) { 
    //std::cout << "initial_element_fn" << std::endl;
    return  ( context.el.index == 0 ); 
}

bool following_element_condition::operator () ( element_context& context ) { 
    //std::cout << "following_element_condition" << std::endl;
    return !( context.el.index == 0 ); 
}

bool following_element_fn       ::operator () ( bool& val, element_context& context ) { 
    //std::cout << "following_element_fn" << std::endl;
    return !( context.el.index == 0 ); 
}

bool top_level_condition        ::operator () ( element_context& context ) { 
    //std::cout << "top_level_condition" << std::endl;
    return  ( context.cl.depth == 0 ); 
}

bool top_level_fn               ::operator () ( bool& val, element_context& context ) { 
    //std::cout << "top_level_fn" << std::endl;
    return  ( context.cl.depth == 0 ); 
}

bool lower_level_condition      ::operator () ( element_context& context ) { 
    //std::cout << "lower_level_condition" << std::endl;
    return !( context.cl.depth == 0 ); 
}

bool lower_level_fn             ::operator () ( bool& val, element_context& context ) { 
    //std::cout << "lower_level_fn" << std::endl;
    return  ( context.cl.depth == 0 ); 
}

bool boundary_condition         ::operator () ( element_context& context ) { 
    bounds( context );
    bounds->pad( context.el.scale );
    return bounds->in_bounds_pad( context.el.position ); 
}

bool boundary_fn                ::operator () ( bool& val, element_context& context ) { 
    return this->operator()( context );
}

bool random_condition::operator () ( element_context& context ) { 
    p( context );
    return rand1( gen ) < *p; 
}

bool random_fn::operator () ( bool& val, element_context& context ) { 
    return this->operator()( context );
}

bool random_sticky_condition::operator () ( element_context& context ) { 
    if( !initialized ) {
        p_start( context );
        on = rand1( gen ) < *p_start;
    }
    else {
        if( on ) {
            p_change_true( context );
            if( rand1( gen ) < *p_change_true ) on = false;
        }
        else {
            p_change_false( context );
            if( rand1( gen ) < *p_change_false ) on = true;
        }
    }
    return on; 
}

bool random_sticky_fn::operator () ( bool& val, element_context& context ) { 
    return this->operator()( context ); 
}

template< class T > bool equal_condition< T >::operator () ( element_context& context ) { 
    a( context );
    b( context );
    return *a == *b;
}

template< class T > bool equal_condition< T >::operator () ( bool& val, element_context& context ) { 
    return this->operator()( context );
}

template struct equal_condition< float >;
template struct equal_condition< vec2f >;
template struct equal_condition< int >;
template struct equal_condition< vec2i >;
template struct equal_condition< frgb >;
template struct equal_condition< ucolor >;
template struct equal_condition< std::string >;
template struct equal_condition< bool >;
template struct equal_condition< direction4 >;
template struct equal_condition< direction8 >;

void filter::operator () ( element_context& context ) { 
    //std::cout << "filter operator ()" << std::endl;
    for( auto& condition : conditions ) {
        //std::cout << "filter condition: " << condition.name << std::endl;
        if( !condition( context ) ) return;
    }
    for( auto& fn : functions ) fn( context );
}

void filter::add_function(  const any_gen_fn      &fn ) { functions.push_back( fn ); }
void filter::add_condition( const any_condition_fn &c ) { conditions.push_back( c ); }

filter::filter() {}

void next_element::add_function(  any_gen_fn fn       ) { functions.push_back( fn ); }

void next_element::add_condition( any_condition_fn c  ) { conditions.push_back( c ); }

bool next_element::operator () ( element_context& context ) { 
    for( auto& condition: conditions ) if( !condition( context ) ) return false; // if any condition fails, no next element
    cluster& cl = context.cl;
    element& el = context.el;
    if( el.index >= *cl.max_n ) return false;    // if max_n reached, no next element
    vec2f p = el.position;                       // save previous position
    for( auto fn : functions ) fn( context );    // calculate functions in order
    if( el.scale < *cl.min_scale ) return false; // if min_scale not reached, no next element
    // calculate derivative - difference between current and previous positions (special case for first element)
    if( el.index != 0 ) el.derivative = el.position - p;
    else el.derivative = rot_deg( el.derivative, el.orientation );
    // bounds check
    el.index++;
    return true; 
}

next_element::next_element() {}


