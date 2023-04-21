#ifndef __SCENE_HPP
#define __SCENE_HPP

#include <variant>
#include <unordered_map>
#include <optional>
#include "image.hpp"
#include "any_image.hpp"
#include "effect.hpp"
#include "next_element.hpp"
#include "any_function.hpp"
#include "any_rule.hpp"

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
};

// An element object contains all the data members needed to create an image splat
struct element {
    // std::vector< effect< T >& > effects; // should effects be generative functions?
    
    vec2f position; 		    // coordinates of element center (relative to parent cluster)
    float scale; 			    // radius of element
    float rotation; 	        // image rotation in degrees
    float orientation;          // direction of motion relative to vector field ( or function )
    bool  orientation_lock;     // is rotation relative to orientation?
    int index;                  // index of element within cluster
    mask_mode mmode;            // how will mask be applied to splat and backround?

    std::string img;  // If no image, element not rendered, serves as placeholder (could make default element white instead)
    std::string mask;
    std::optional< any_pixel > tint;	// change the color of element

    // approximate absolute derivative used to calculate angle of branches
    // calculated from delta since last position or directly (for instance in the case of a circle)
    vec2f derivative;   // move to element_context?
    bool derivative_lock;

    void render( any_buffer_pair_ptr& target, element_context& context );
    void operator () ( any_buffer_pair_ptr& buf, element_context& context );    // single element effect

    // needed?
    // render into a buffer pair. Rendering modifies image directly - does not require buffer swap.
    // in this case the element serves as an effect functor

    element(    const vec2f& position_init =  { 0.0f, 0.0f },
                const float& scale_init = 1.0f,
                const float& rotation_init = 0.0f,
                const float& orientation_init = 0.0f,
                const bool   orientation_lock_init = false,
                const vec2f& derivative_init = { 1.0f, 0.0f },
                const bool   derivative_lock_init = false,
                std::string  img_init  = "none",
                std::string  mask_init = "none",
                const std::optional< any_pixel > tint_init = std::nullopt,
                const mask_mode mmode_init = MASK_BLEND           
            ) 
        : position( position_init ),
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
};

// A default cluster with root set to default element with a single image should produce a full frame image, e.g. for background
struct cluster {
    element root_elem;       // initial element in cluster
    next_element& next_elem; // Functor to recursively generate elements in cluster

    int max_n;          // limit to number of elements
    int depth;          // counter to keep track of depth in tree
    int max_depth;      // prevent infinite recursion
    float min_scale;    // approximately one pixel
    bool background_dependent;  // if true, cluster will double buffer on rendering

    std::optional< bb2f > bounds;   // Optionally, cluster will stop generating if it goes out of bounds

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
             const float& min_scale_init = 0.01f,
             const bool& background_dependent_init = false,
             const std::optional< bb2f >& bounds_init = std::nullopt
            )
        : root_elem( el ),
          next_elem( next_elem_init ),
          max_n( max_n_init ),
          depth( depth_init ),
          max_depth( max_depth_init ),
          min_scale( min_scale_init ),
          background_dependent( background_dependent_init ),
          bounds( bounds_init ) 
          {}
};

typedef enum {
    MODE_STATIC,        // render once
    MODE_ITERATIVE,    // render every frame based on previous frame
    MODE_EPHEMERAL      // render every frame starting with background
} render_mode;

// An effect list contains a list of effects to be rendered in sequence
struct effect_list {
    render_mode rmode;          // how will effects be rendered?
    std::string source_name;    // name of source image or buffer (if name not in list assume blank source)
    std::string name;
    any_buffer_pair_ptr buf;
    std::vector< std::string > effects;
    pixel_type ptype;   // pixel type of buffer
    vec2i dim;          // dimensions of buffer
    float relative_dim; // proportion of buffer size to use for effect - deferred
    bool rendered;      // Has static buffer already been rendered? Set to false after changing any effect

    any_image_ptr get_initial_image();

    void render( scene& s );
    void resize( const vec2i& new_dim );

    effect_list( const std::string& name_init = "default",
                 const std::string& source_name_init = "none",
                 const vec2i& dim_init = { 0, 0 },
                 const pixel_type& ptype_init = PIXEL_UCOLOR, 
                 const render_mode& rmode_init = MODE_STATIC, 
                 const float& relative_dim_init = 1.0f) :
    name( name_init ),
    source_name( source_name_init ),
    dim( dim_init ),
    ptype( ptype_init ), 
    rmode( rmode_init ), 
    relative_dim( relative_dim_init ),
    rendered( false ) 
    {
        switch( ptype ) {
            case( PIXEL_FRGB   ): buf = std::make_shared< buffer_pair< frgb >   >( dim ); break;
            case( PIXEL_UCOLOR ): buf = std::make_shared< buffer_pair< ucolor > >( dim ); break;
            case( PIXEL_VEC2F  ): buf = std::make_shared< buffer_pair< vec2f >  >( dim ); break;
            case( PIXEL_INT    ): buf = std::make_shared< buffer_pair< int >    >( dim ); break;
            case( PIXEL_VEC2I  ): buf = std::make_shared< buffer_pair< vec2i >  >( dim ); break;
        }
    }
};

struct scene {
    // scene owns clusters, elements, images, effects, and functions
    std::string name;

    std::unordered_map< std::string, any_fn< float  > > float_fns;    
    std::unordered_map< std::string, any_fn< int    > > int_fns;    
    std::unordered_map< std::string, any_fn< vec2f  > > vec2f_fns;    
    std::unordered_map< std::string, any_fn< vec2i  > > vec2i_fns;
    std::unordered_map< std::string, any_fn< frgb   > > frgb_fns;
    std::unordered_map< std::string, any_fn< ucolor > > ucolor_fns;
    std::unordered_map< std::string, any_fn< bb2i   > > bb2i_fns;
    std::unordered_map< std::string, any_fn< bb2f   > > bb2f_fns;
    std::unordered_map< std::string, any_gen_fn       > gen_fns;
    std::unordered_map< std::string, any_condition_fn > condition_fns;

    //std::unordered_map< std::string, any_image_ptr > images; // images now stored in buffers map
    std::unordered_map< std::string, std::shared_ptr< element > > elements;
    std::unordered_map< std::string, std::shared_ptr< next_element > > next_elements; // next element functions tagged with cluster names
    std::unordered_map< std::string, std::shared_ptr< cluster > > clusters;           // scene defined as a set of clusters
    std::unordered_map< std::string, any_rule > CA_rules;                             // rules for cellular automata
    std::unordered_map< std::string, any_effect_fn > effects;

    std::unordered_map< std::string, any_buffer_pair_ptr > buffers; // Source images and results of effect stacks
    std::vector< effect_list > queue; // list of buffers rendered in order - last in list displayed on screen (should be named "display" )

    float time; 
    float time_interval; 
    float default_time_interval;
    bool paused;
    float aspect;   // aspect ratio of output buffer
        
    scene( float time_interval_init = 1.0f );                                // create empty scene object
    scene( const std::string& filename, float time_interval_init = 1.0f );   // Load scene file (JSON) into new scene object

    bool load( const std::string& filename );   // Load scene file (JSON) into existing scene object
    void pause();                               // Pause animation
    void unpause();                             // Set animation to runnning
    void run_pause();                           // Toggle animation pause
    void set_time_interval( const float& t );   // Set time interval for animation
 // void resize( const vec2i& dim );            // Resize all buffers in render list
    void set_output_buffer( any_buffer_pair_ptr& buf ); // Set output buffer for rendering (needed?)
    effect_list& get_effect_list( const std::string& name ); // get effect list by name

    void render();  // Render scene on any image type

    // Render and save to file
    void render_and_save(    
        const std::string& filename, 
        const vec2i& dim = { 512, 512 },
        pixel_type ptype = PIXEL_UCOLOR, 
        file_type ftype = FILE_JPG, 
        int quality = 100 
    );

    void animate( std::string basename, int nframes = 100, vec2i dim = { 512, 512 } );
};

#endif // __SCENE_HPP
