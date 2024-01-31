#include "UI.hpp"
#include "image.hpp"
#include "next_element.hpp"
#include "scene.hpp"

bool mousedown_condition::operator () ( element_context& context ) { 
    return context.s.ui.mouse_down; 
}

bool mousedown_fn::operator () ( bool& val, element_context& context ) { 
    return context.s.ui.mouse_down; 
}

bool mouseover_condition::operator () ( element_context& context ) { 
    return context.s.ui.mouse_over; 
}

bool mouseover_fn::operator () ( bool& val, element_context& context ) { 
    return context.s.ui.mouse_over; 
}

bool mouseclick_condition::operator () ( element_context& context ) { 
    return context.s.ui.mouse_click; 
}

bool mouseclick_fn::operator () ( bool& val, element_context& context ) { 
    return context.s.ui.mouse_click; 
}

bool switch_condition::operator () ( element_context& context ) { 
    return value; 
}

bool switch_fn::operator () ( bool& val, element_context& context ) { 
    return value; 
}

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
    return choice;
}

std::string menu::operator () ( std::string& val, element_context& context ) {
    return get_chosen_item();
}

std::string menu::get_chosen_item() {
    return items[ choice ];
}

void menu::choose( int choice ) {
    this->choice = choice;
}

void menu::choose( const std::string& name ) {
    for( int i = 0; i < items.size(); i++ ) {
        if( items[ i ] == name ) {
            choice = i;
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

template< class T > T direction_picker< T >::operator() ( T& val, element_context& context ) {
    return value;
}

template< class T > void direction_picker< T >::reset() {
    value = default_value;
}

template struct direction_picker< direction4 >;
template struct direction_picker< direction8 >;

void widget_switch::reset() {
    if ( switcher ) switcher->reset();
    std::visit( [&]( auto& w ) { if( w ) w->reset(); }, widget );
}

bool widget_switch::operator() ( element_context& context ) {
    if ( switcher ) {
        switcher->operator()( context );
        return switcher->value;
    }
    else return true; // no switcher, defaults to just the widget
}

bool widget_switch::operator() ( bool& val, element_context& context ) {
    if ( switcher ) {
        switcher->operator()( val, context );
        return switcher->value;
    }
    else return true; // no switcher, defaults to just the widget
}

void widget_group::add_widget( const any_widget_ptr& widget ) {
    widgets.push_back( widget );
}

void widget_group::reset() {
    for( auto& widget : widgets ) {
        std::visit( [&]( auto& w ) { w->reset(); }, widget );
    }
}

void widget_group::clear() {
    widgets.clear();
}

void UI::add_widget_group( const std::string& name, const widget_group& wg ) {
    widget_groups[ name ] = wg;
}
