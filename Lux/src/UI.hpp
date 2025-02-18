#ifndef UI_HPP
#define UI_HPP

#include <string>
#include <variant>
#include <unordered_map>
#include "joy_concepts.hpp"
#include "joy_rand.hpp"
#include "vect2.hpp"
#include "image.hpp"
#include "next_element.hpp"

typedef enum menu_type
{
    MENU_PULL_DOWN,
    MENU_RADIO,
    MENU_IMAGE
} menu_type;

typedef enum switch_type
{
    SWITCH_SWITCH,
    SWITCH_CHECKBOX
} switch_type;

struct element_context;

struct mousedown_condition {
    bool operator () ( element_context& context );
    bool operator () ( bool& val, element_context& context );
};
typedef mousedown_condition mousedown_fn;

struct mouseover_condition {
    bool operator () ( element_context& context );
    bool operator () ( bool& val, element_context& context );
};
typedef mouseover_condition mouseover_fn;

struct mouseclick_condition {
    bool operator () ( element_context& context );
    bool operator () ( bool& val, element_context& context );
};
typedef mouseclick_condition mouseclick_fn;

struct switch_condition {
    switch_type tool;
    std::string label, description;
    bool value, default_value;
    bool affects_widget_groups;

    bool operator () ( element_context& context );
    bool operator () ( bool& val, element_context& context ); 

    void reset(); // reset value to default

    switch_condition(   const switch_type& tool_init = SWITCH_SWITCH,
                        const std::string& label_init = "", 
                        const std::string& description_init = "", 
                        const bool& default_value_init = true,
                        const bool& affects_widget_groups_init = false ) : 
                            tool( tool_init ),
                            label( label_init ),
                            description( description_init ),
                            default_value( default_value_init ),
                            value( default_value_init ),
                            affects_widget_groups( affects_widget_groups_init )
                            {}
};

typedef switch_condition switch_fn;

template< Scalar T > struct slider {
    std::string label, description;
    T value, default_value, min, max, step;

    T operator () ( T& val, element_context& context ); 

    void reset(); // reset value to default

    slider( const std::string& label_init = "", 
            const std::string& description_init = "",
            const T& min_init = T( 0 ), 
            const T& max_init = T( 100 ), 
            const T& default_value_init = T( 50 ), 
            const T& step_init = T( 1 ) ) :
                label( label_init ), 
                description( description_init ),
                value( default_value_init ), 
                default_value( default_value_init ),
                min( min_init ), 
                max( max_init ), 
                step( step_init ) {}
};

typedef slider< float > slider_float;
typedef slider< int >   slider_int;

template< Scalar T > struct range_slider {
    std::string label, description;
    T min, max, step;
    interval< T > value, default_value;

    interval< T > operator () ( interval< T >& val, element_context& context );

    void reset(); // reset value to default

    range_slider( const std::string& label_init = "", 
            const std::string& description_init = "",
            const T& min_init = T( 0 ), 
            const T& max_init = T( 100 ), 
            const interval< T >& default_value_init = interval< T >( 0, 100 ), 
            const T& step_init = T( 1 ) ) :
                label( label_init ), 
                description( description_init ),
                value( default_value_init ), 
                default_value( default_value_init ),
                min( min_init ), 
                max( max_init ), 
                step( step_init ) {}
};

typedef range_slider< float > range_slider_float;
typedef range_slider< int >   range_slider_int;

struct menu {
    menu_type tool;
    std::string label, description;
    std::vector< std::string > items;
    int choice, default_choice;
    //bool user_defined_item;
    bool affects_widget_groups;
    bool rerender;

    int operator () ( int& val, element_context& context ); 
    std::string operator () ( std::string& val, element_context& context );

    std::string get_chosen_item();
    void choose( int choice );
    void choose( const std::string& name );
    void add_item( const std::string& item );
    void reset(); // reset choice to default
    void clear(); // clear all items

    menu( const std::string& label_init = "", 
          const std::string& description_init = "", 
          const int& default_choice_init = 0,
          const menu_type& tool_init = MENU_PULL_DOWN,
          //const bool& user_defined_item_init = false,
          const bool& affects_widget_groups_init = false,
          const bool& rerender_init = false ) : 
            label( label_init ), 
            description( description_init ), 
            choice( default_choice_init ), 
            default_choice( default_choice_init ),
            tool( tool_init ),
            //user_defined_item( user_defined_item_init ),
            affects_widget_groups( affects_widget_groups_init ),
            rerender( rerender_init )
        {}
};

typedef menu menu_int;
typedef menu menu_string;

template< class T > struct picker {
    std::string label, description;
    T value, default_value;

    T operator () ( T& val, element_context& context ); 

    void reset(); // reset value to default

    picker( const T& default_value_init = T(),
                     const std::string& label_init = "", 
                     const std::string& description_init = "" 
                       ) : 
                        label( label_init ), 
                        description( description_init ), 
                        value( default_value_init ), 
                        default_value( default_value_init ) {}
};

typedef picker< funk_factor > funk_factor_picker;
typedef picker< direction4 > direction_picker_4;
typedef picker< direction4_diagonal > direction_picker_4_diagonal;
typedef picker< direction8 > direction_picker_8;
typedef picker< box_blur_type > box_blur_picker;
typedef picker< image_extend > image_extend_picker;
typedef picker< int > multi_direction8_picker; // bytecode for 8 directions

// Custom blur picker is an int harness function that contains multiple direction pickers
struct custom_blur_picker {
    std::string label, description;
    // generated multi direction pickers wrapped in any_fn 
    // first is set of directions for blur, second is set of directions for comparison
    std::vector< std::pair< int, int > > pickers;    

    // returns size of dirs_pickers vector
    int operator () ( int& val, element_context& context ); 

    void add_pickers(); // push_back a new picker to dirs_pickers and compares_pickers
    void add_pickers( int dirs, int compares ); // push_back a new picker to dirs_pickers and compares_pickers, setting values
    void remove_pickers( int index ); // remove dir and compare picker at index
    void set_picker_value( int value, int id ); // set value of dir or compare picker identified by id
    void reset(); // reset value to default
    void clear(); // clear all items

    custom_blur_picker( const std::string& label_init = "", 
                        const std::string& description_init = "" );
};

// Widget groupings

// a checkbox or toggle switch activates or deactivates a widget
struct widget_switch {  
    std::string switcher;
    std::string widget;
    std::string label, description;

    bool operator () ( element_context& context );
    bool operator () ( bool& val, element_context& context );

    widget_switch( const std::string& switcher_init = "", const std::string& widget_init = "", const std::string& label_init = "", const std::string& description_init = "" ) : switcher( switcher_init ), widget( widget_init ), label( label_init ), description( description_init ) {}
};

typedef widget_switch widget_switch_condition;
typedef widget_switch widget_switch_fn;

struct widget_group {
    std::string name, label, description;
    std::vector< std::string > conditions;
    std::vector< std::string > widgets;
    // bool open;      // is this widget group open? (Needed?)
    // bool active;    // is this widget group within active part of scene graph? (Needed?)

    bool operator () ( element_context& context );
    void add_widget( const std::string& widget );
    void add_condition( const std::string& condition );
//    void reset(); // reset all widgets to default
    void clear(); // clear all widgets

    widget_group( const std::string& name_init = "", const std::string& label_init = "", const std::string& description_init = "" );
    ~widget_group();
};

// Stores state of user interface - mouse position, mouse down, slider values, etc.
struct UI {
    bool running;
    bool displayed;
    bool advance;

    bb2i  canvas_bounds; // bounds of canvas in pixels
    vec2i mouse_pixel;  // mouse position in pixels

    bool  mouse_down;
    bool  mouse_over;
    bool  mouse_click;  // true for one frame when mouse clicked over canvas
    std::vector< widget_group > widget_groups;  // should this be a tree?

    void add_widget_group( const std::string& name, const widget_group& wg );

    UI() : 
        running( true ),
        displayed( false ),
        advance( false ),
        canvas_bounds( bb2i( { 0, 0 }, { 512, 512 } ) ), 
        mouse_pixel( { 0, 0 } ), 
        mouse_down( false ), 
        mouse_over( false ), 
        mouse_click( false ) {}
};

#endif // UI_HPP
