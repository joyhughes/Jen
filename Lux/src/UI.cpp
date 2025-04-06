#include "UI.hpp"
#include "image.hpp"
//#include "next_element.hpp"
#include "scene.hpp"
#include "life.hpp"
#include "any_function.hpp"

bool mousedown_condition::operator () ( element_context& context ) { 
    return context.s.ui.mouse_down; 
}

bool mousedown_condition::operator () ( bool& val, element_context& context ) { 
    return context.s.ui.mouse_down; 
}

bool mouseover_condition::operator () ( element_context& context ) { 
    return context.s.ui.mouse_over; 
}

bool mouseover_condition::operator () ( bool& val, element_context& context ) { 
    return context.s.ui.mouse_over; 
}

bool mouseclick_condition::operator () ( element_context& context ) { 
    return context.s.ui.mouse_click; 
}

bool mouseclick_condition::operator () ( bool& val, element_context& context ) { 
    return context.s.ui.mouse_click; 
}

bool switch_condition::operator () ( element_context& context ) { 
    return value; 
}

bool switch_condition::operator () ( bool& val, element_context& context ) { 
    return value; 
}

void switch_condition::reset() { value = default_value; }

template< Scalar T > void slider< T >::reset() { value = default_value; }

template< Scalar T > T slider< T >::operator () ( T& val, element_context& context ) {
    return value;
}

template struct slider< float >;
template struct slider< int >;

template< Scalar T > void range_slider< T >::reset() { value = default_value; }

template< Scalar T > interval< T > range_slider< T >::operator () ( interval< T >& val, element_context& context ) {
    return value;
}

template struct range_slider< float >;
template struct range_slider< int >;

int menu::operator () ( int& val, element_context& context ) {
    //std::cout << "Menu operator () (int): nitems=" << items.size() << " choice=" << choice << "\n";
    return choice;
}

std::string menu::operator () ( std::string& val, element_context& context ) {
    //std::cout << "Menu operator () (int): nitems=" << items.size() << " val=" << val << "\n";
    return get_chosen_item();
}

std::string menu::get_chosen_item() {
    return items[ choice ];
}

void menu::choose( int c ) {
    //std::cout << "Menu choose(int): nitems=" << items.size() << " choice=" << choice << "\n";
    choice = c;
}

void menu::choose( const std::string& name ) {
    std::cout << "Menu choose(string): nitems=" << items.size() << " name=" << name << "\n";
    for( int i = 0; i < items.size(); i++ ) {
        if( items[ i ] == name ) {
            choice = i;
            std::cout << "Menu choose(string): found name=" << name << " choice=" << choice << "\n";
            return;
        }
    }
}

void menu::add_item( const std::string& item ) {
    items.push_back( item );
}

void menu::reset() {
    choice = default_choice;
}

void menu::clear() {
    items.clear();
}

template< class T > T picker< T >::operator() ( T& val, element_context& context ) {
    return value;
}

template< class T > void picker< T >::reset() {
    value = default_value;
}

template struct picker< direction4 >;
template struct picker< direction4_diagonal >;
template struct picker< direction8 >;
template struct picker< box_blur_type >;
template struct picker< int >;

int custom_blur_picker::operator() ( int& val, element_context& context ) {
    return pickers.size();
}

void custom_blur_picker::reset() {
    for( auto& p : pickers ) {
        p.first =  0;
        p.second = 0;
    }
}

void custom_blur_picker::clear() {
    pickers.clear();
}

void custom_blur_picker::add_pickers() {
    pickers.push_back( std::make_pair( 0, 0 ) );
}

void custom_blur_picker::add_pickers( int dirs, int compares) { 
    pickers.push_back( std::make_pair( dirs, compares ) );
}

void custom_blur_picker::remove_pickers( int n ) {
    pickers.erase( pickers.begin() + n );
}

void custom_blur_picker::set_picker_value( int value, int id ) {
    // msb = 1 means compare picker, 0 means direction picker
    if( ( id >> 7 ) & 1 ) {
        pickers[ id & 0x7F ].second = value;
    }
    else {
        pickers[ id & 0x7F ].first  = value;
    }
}

custom_blur_picker::custom_blur_picker( const std::string& label_init, 
                                        const std::string& description_init ) : 
                                            label( label_init ), 
                                            description( description_init ) {
                                                // initialize pickers
                                            }

bool widget_switch::operator() ( element_context& context ) {
    auto sw = context.s.get_fn_ptr< bool, switch_fn >( switcher );
    if ( sw ) {
        sw->operator()( context );
        return sw->value;
    }
    else return true; // no switcher, defaults to just the widget
}

bool widget_switch::operator() ( bool& val, element_context& context ) {
    auto sw = context.s.get_fn_ptr< bool, switch_fn >( switcher );
    if ( sw ) {
        sw->operator()( context );
        return sw->value;
    }
    else return true; // no switcher, defaults to just the widget
}

void widget_group::add_widget( const std::string& widget ) {
    widgets.push_back( widget );
}

void widget_group::add_condition( const std::string& condition ) {
    conditions.push_back( condition );
}

void widget_group::clear() {
    widgets.clear();
}

bool widget_group::operator() ( element_context& context ) {
    for( auto& c : conditions ) {
        auto& cfn = std::get< any_condition_fn >( context.s.functions[ c ] );
        if( !cfn.fn( context ) ) return false;
    }
    return true;
}

widget_group::widget_group( const std::string& name_init, const std::string& label_init, const std::string& description_init ) : name( name_init ), label( label_init ), description( description_init ) {}

widget_group::~widget_group() {
    clear();
}

void UI::add_widget_group( const std::string& name, const widget_group& wg ) {
    widget_groups.push_back( wg );
}

