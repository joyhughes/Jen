#ifndef __SCENE_HPP
#define __SCENE_HPP

#include <variant>
#include <unordered_map>
#include <optional>
#include "buffer_pair.hpp"
#include "any_image.hpp"
#include "effect.hpp"
#include "next_element.hpp"
#include "any_function.hpp"
#include "any_rule.hpp"
#include "UI.hpp"
#include "json.hpp"
#include <mutex>
#include <set>

using json = nlohmann::json;

//template< class T > struct effect;
struct element;
struct cluster;
struct scene;

struct element_context {
    element& el;
    cluster& cl;
    scene& s;
    any_buffer_pair_ptr& buf;  // buffer pair being rendered upon - access to original image and image being rendered
    //float t;  // time

    element_context( element& el_init, cluster& cl_init, scene& s_init, any_buffer_pair_ptr& buf_init ) :
        el( el_init ), cl( cl_init ), s( s_init ), buf( buf_init ) {}

    element_context( scene& s_init, any_buffer_pair_ptr& buf_init );
};

// An element object contains all the data members needed to create an image splat
struct element {
    // std::vector< effect< T >& > effects; // should effects be generative functions?
    bool smooth;                // should image be smoothed?
    vec2f position; 		    // coordinates of element center (relative to parent cluster)
    float scale; 			    // radius of element
    float rotation; 	        // image rotation in degrees
    float orientation;          // direction of motion relative to vector field ( or function )
    bool  orientation_lock;     // is rotation relative to orientation?
    int index;                  // index of element within cluster
    mask_mode mmode;            // how will mask be applied to splat and backround?

    any_buffer_pair_ptr img;  // If no image, element not rendered, serves as placeholder (could make default element white instead)
    any_buffer_pair_ptr mask;
    std::optional< any_pixel > tint;	// change the color of element

    // approximate absolute derivative used to calculate angle of branches
    // calculated from delta since last position or directly (for instance in the case of a circle)
    vec2f derivative;   // move to element_context?
    bool derivative_lock;

    void render( any_buffer_pair_ptr& target );
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );    // single element effect

    // needed?
    // render into a buffer pair. Rendering modifies image directly - does not require buffer swap.
    // in this case the element serves as an effect functor

    element(    const bool& smooth_init = false,
                const vec2f& position_init =  { 0.0f, 0.0f },
                const float& scale_init = 1.0f,
                const float& rotation_init = 0.0f,
                const float& orientation_init = 0.0f,
                const bool   orientation_lock_init = false,
                const vec2f& derivative_init = { 1.0f, 0.0f },
                const bool   derivative_lock_init = false,
                any_buffer_pair_ptr  img_init  = null_buffer_pair_ptr,
                any_buffer_pair_ptr  mask_init = null_buffer_pair_ptr,
                const std::optional< any_pixel > tint_init = std::nullopt,
                const mask_mode mmode_init = MASK_BLEND           
            ) 
        :   smooth( smooth_init ),
            position( position_init ),
            scale( scale_init ),
            rotation( rotation_init ),
            orientation( orientation_init ),
            orientation_lock( orientation_lock_init ),
            derivative( derivative_init ),
            derivative_lock( derivative_lock_init ),
            img( img_init ),
            mask( mask_init ),
            tint( tint_init ),
            mmode( mmode_init ),
            index( 0 ) {}

    // copy constructor
    element( const element& el ) :  
        position( el.position ),
        scale( el.scale ),
        rotation( el.rotation ),
        orientation( el.orientation ),
        orientation_lock( el.orientation_lock ),
        derivative( el.derivative ),
        derivative_lock( el.derivative_lock ),
        img( el.img ),
        mask( el.mask ),
        tint( el.tint ),
        mmode( el.mmode ),
        index( el.index ) 
        {}
};

static element default_element = element();

// A default cluster with root set to default element with a single image should produce a full frame image, e.g. for background
struct cluster {
    element root_elem;       // initial element in cluster
    next_element& next_elem; // Functor to recursively generate elements in cluster

    harness< int > max_n;          // index of cluster within scene
    int depth;                     // counter to keep track of depth in tree
    harness< int > max_depth;      // prevent infinite recursion by limiting depth of tree
    harness< float > min_scale;    // prevent infinite recursion by limiting scale of elements
    bool background_dependent;     // if true, cluster will double buffer on rendering

    // Recursively generate branches and render elements
    void render( scene& s, any_buffer_pair_ptr& img );

    // change root element parameters for branching cluster
    void set_root( element& el );

    // render into a buffer pair. Rendering modifies image directly - usually does not require 
    // buffer swap, but could if cluster generation depends on background image.
    // in this case the cluster serves as an effect functor (the effect being rendering)
    // only at top level cluster. Effects would need to reference background image directly
    // need generic buffer pair
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );

    cluster( const element& el,  
             next_element& next_elem_init, 
             const int& max_n_init = 1,
             const int& depth_init = 0,
             const int& max_depth_init = 10,
             const bool& background_dependent_init = false
            )
        : root_elem( el ),
          next_elem( next_elem_init ),
          max_n( max_n_init ),
          depth( depth_init ),
          max_depth( max_depth_init ),
          background_dependent( background_dependent_init )
          {}

    // copy constructor
    cluster( const cluster& cl ) :  
        root_elem( cl.root_elem ),
        next_elem( cl.next_elem ),
        depth( cl.depth ),
        max_depth( cl.max_depth ),
        background_dependent( cl.background_dependent )
        {}        
};

static next_element default_next_element = next_element();
static cluster default_cluster = cluster( default_element, default_next_element );

typedef enum {
    MODE_STATIC,        // render once
    MODE_ITERATIVE,     // render every frame based on previous frame
    MODE_EPHEMERAL      // render every frame starting with background
} render_mode;
 
// An effect list contains a list of effects to be rendered in sequence
struct effect_list {
    render_mode rmode;          // how will effects be rendered?
    bool self_generated;        // If false, copy buffer with source name. If true, generate 
    harness< std::string > source_name;    // name of source image or buffer. Used as reference size for self generated buffers. Will resize buffer if necessary when name changes.
    std::string name;
    any_buffer_pair_ptr buf;
    std::vector< std::string > effects;
    pixel_type ptype;   // pixel type of buffer
    harness< vec2i > dim;          // dimensions of buffer
    float relative_dim; // proportion of buffer size to use for effect - deferred
    bool rendered;      // Has static buffer already been rendered? Set to false after changing any effect

    void update( scene& s );
    //void update_source_name( scene& s );
    //void copy_source_buffer( scene& s );
    void render( scene& s );
    void resize( vec2i new_dim );
    void restart( scene& s );   // return to initial condition

    effect_list( const std::string& name_init = "default",
                 const bool& self_generated_init = false,
                 const std::string& source_name_init = "none",
                 const vec2i& dim_init = { 512, 512 },
                 const pixel_type& ptype_init = PIXEL_UCOLOR, 
                 const render_mode& rmode_init = MODE_STATIC, 
                 const float& relative_dim_init = 1.0f) : 
    name( name_init ),
    self_generated( self_generated_init ),
    source_name( source_name_init ),
    dim( dim_init ),
    ptype( ptype_init ), 
    rmode( rmode_init ), 
    relative_dim( relative_dim_init ),
    rendered( false ) 
    {
        //std::cout << "dim = " << *dim << std::endl;
        switch( ptype ) {
            case( PIXEL_FRGB   ):   
                buf = std::make_shared< buffer_pair< frgb >   >( dim_init ); 
                break;
            case( PIXEL_UCOLOR ):
                buf = std::make_shared< buffer_pair< ucolor > >( dim_init ); 
                break;
            case( PIXEL_VEC2F ):
                buf = std::make_shared< buffer_pair< vec2f >  >( dim_init ); 
                break;
            case( PIXEL_INT    ): 
                buf = std::make_shared< buffer_pair< int >    >( dim_init ); 
                break;
            case( PIXEL_VEC2I  ): 
                buf = std::make_shared< buffer_pair< vec2i >  >( dim_init ); 
                break;
        }
    }
};

struct scene {
    // scene owns clusters, elements, images, effects, and functions
    std::string name;
    bool liveCamera;
    UI ui;  // We gotta go now

    std::unordered_map< std::string, any_function > functions; 
    std::unordered_map< std::string, std::shared_ptr< element > > elements;
    std::unordered_map< std::string, std::shared_ptr< next_element > > next_elements; // next element functions tagged with cluster names
    std::unordered_map< std::string, std::shared_ptr< cluster > > clusters;           
    std::unordered_map< std::string, any_rule > CA_rules;                             // rules for cellular automata
    std::unordered_map< std::string, any_effect_fn > effects;

    std::unordered_map< std::string, any_buffer_pair_ptr > buffers; // Source images and results of effect stacks
    std::vector< effect_list > queue; // list of buffers rendered in order of execution

    float time; 
    float time_interval; 
    float default_time_interval;
    
    float aspect;   // aspect ratio of output buffer
        
    scene( float time_interval_init = 1.0f );                                // create empty scene object
    scene( const std::string& filename, float time_interval_init = 1.0f );   // Load scene file (JSON) into new scene object
public:
    json save_complete_state() const;
    void load_complete_state(const json& state);

    json capture_current_frame_state() const;
    json capture_state_delta(const json& base_line) const;
    void restore_live_state(const json&live_state);

    json save_runtime_state() const;
    void restore_runtime_state(const json& state);

    void update_frame();
    void mark_function_changed(const std::string& fname);
    float calculate_delta_time() const;

    void on_slider_changed(const std::string& slider_name, const float& new_value);
    void on_menu_changed(const std::string& menu_name, const int new_choice);
    void on_switch_toggled(const std::string& switch_name, const bool new_state);

private:
    // thread-safety for live capture
    mutable std::mutex state_mutex;
    std::atomic<bool> capture_enabled{true};
    std::chrono::steady_clock::time_point last_capture_time;
    std::chrono::steady_clock::time_point last_frame_time;

    // live animation state
    uint64_t current_frame_number{0};
    float current_fps{60.f};
    bool ui_recently_modified{false};
    std::set<std::string> functions_changed_this_frame;

    // helper methods for harness serialization

    json get_buffer_state(const std::string& buffer_name) const;
    void restore_buffer_state(const std::string& buffer_name, const json& buffer_json);

    // live animation helpers
    json capture_effect_chain_states() const;
    json capture_buffer_render_states() const;
    void update_audio_functions();
    void update_generator_functions();
    void update_integrator_functions();
    void propogate_all_harness_updates();
    void reset_user_interaction_flags();
    bool should_capture_state_this_frame() const;
    void background_capture_state();

    // state validation
    void validate_loaded_state();
    void sync_ui_with_function_states();







    template< class T, class F > std::shared_ptr< F > get_fn_ptr( const std::string& name ) {
        if( !functions.contains( name ) ) {
            throw std::runtime_error( "function " + name + " not found in scene" ); 
            return nullptr;
        }
        else {
            if( !std::holds_alternative< any_fn< T > >( functions[ name ] ) ) {
                throw std::runtime_error( "function " + name + " not of return type " + typeid( T ).name() );
                return nullptr;
            }
            else {
                auto fn = std::get< any_fn< T > >( functions[ name ] );
                if( !std::holds_alternative< std::shared_ptr< F > >( fn.any_fn_ptr ) ) {
                    throw std::runtime_error( "function " + name + " not of type " + typeid( F ).name() );
                    return nullptr;
                }
                else {
                    return std::get< std::shared_ptr< F > >( fn.any_fn_ptr );
                }
            }
        }
    }

    template< class T > image< T >& get_image( const std::string& name ) {
        if( !buffers.contains( name ) ) {
            throw std::runtime_error( "image " + name + " not found in scene" ); 
        }
        else return ::get_image< T >( buffers[ name ] );
    }

    // Get mouse position in parametric space of output buffer
    vec2f get_mouse_pos() const;
    std::string get_current_scene_name();
    void load_scene_by_name(const std::string& name);

//    bool load( const std::string& filename );   // Load scene file (JSON) into existing scene object
//    void pause();                               // Pause animation
//    void unpause();                             // Set animation to runnning
//    void run_pause();                           // Toggle animation pause
    void restart();                             // Reset time to zero, reload all buffers
    void set_time_interval( const float& t );   // Set time interval for animation
 // void resize( const vec2i& dim );            // Resize all buffers in render list
    void set_output_buffer( any_buffer_pair_ptr& buf ); // Set output buffer for rendering (needed?)
    effect_list& get_effect_list( const std::string& name ); // get effect list by name

    void render();  // Render scene on any image type

    void save_result(    
        const std::string& filename, 
        const vec2i& dim = { 512, 512 },
        pixel_type ptype = PIXEL_UCOLOR, 
        file_type ftype = FILE_JPG, 
        int quality = 100 
    );

    // Render and save to file
    void render_and_save(    
        const std::string& filename, 
        const vec2i& dim = { 512, 512 },
        pixel_type ptype = PIXEL_UCOLOR, 
        file_type ftype = FILE_JPG, 
        int quality = 100 
    );          

    void animate( 
        std::string basename, 
        int nframes = 100, 
        vec2i dim = { 512, 512 },
        pixel_type ptype = PIXEL_UCOLOR, 
        file_type ftype = FILE_JPG, 
        int quality = 100   
    );


};


/*
static element empty_element;
static next_element empty_next_element;
static cluster empty_cluster( empty_element, empty_next_element );
static scene empty_scene;
static element_context empty_element_context( empty_element, empty_cluster, empty_scene, any_buffer_pair_ptr( null_buffer_pair_ptr ) );
*/
// Test only - remove later
//template< class T > void splat_element( std::shared_ptr< buffer_pair< T > > target_buf, element& el );


#endif // __SCENE_HPP