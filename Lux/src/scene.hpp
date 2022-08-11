#ifndef __SCENE_HPP
#define __SCENE_HPP

#include <variant>
#include <map>
#include <optional>
#include "image.hpp"
#include "fimage.hpp"

//template< class T > struct effect;
template< class T > struct element;
template< class T > struct cluster;

template< class T > struct element_context {
    element< T >& el;
    cluster< T >& cl;
    image< T >& img;
    float t;  // time
    // scene scn;
    // derivative?

    element_context( element< T >& el_init, cluster< T >& cl_init, image< T >& img_init, const float& t_init = 0.0f ) :
        el( el_init ), cl( cl_init ), img( img_init ), t( t_init ) {}
};

// An element object contains all the data members needed to create an image splat
template< class T > struct element {
    image< T >& img;
    // std::vector< effect< T >& > effects; // should effects be generative functions?
    
    vec2f position; 		    // coordinates of element center (relative to parent cluster)
    float scale; 			    // radius of element
    float rotation; 	        // image rotation in degrees
    float orientation;          // direction of motion relative to vector field ( or function )
    bool  orientation_lock;     // is rotation relative to orientation?
    std::optional< T > tint;	// change the color of element
    image< T >* mask;
    int index;

    // approximate absolute derivative used to calculate angle of branches
    // calculated from delta since last position or directly (for instance in the case of a circle)
    vec2f derivative;   // move to element_holder?
    bool derivative_lock;

    void render( image< T >& in, const float& t );

    // render into a buffer pair. Rendering modifies image directly - does not require buffer swap.
    // in this case the element serves as an effect functor 
    void operator () ( buffer_pair< T >& buf, const float& t ); // render into a buffer pair. Rendering does not require buffer swap.

    element(    image< T >& img_init, 
                const vec2f& position_init =  { 0.0f, 0.0f },
                const float& scale_init = 1.0f,
                const float& rotation_init = 0.0f,
                const float& orientation_init = 0.0f,
                const std::optional< T >& tint_init = std::nullopt,
                image< T >* mask_init = NULL 
            ) : 
                img( img_init ),
                position( position_init ),
                scale( scale_init ),
                rotation( rotation_init ),
                orientation( orientation_init ),
                tint( tint_init ),
                mask( mask_init ),
                index( 0 ) {}
};

template< class T > struct next_element;

template< class T > struct cluster {
    element< T > root_elem;       // initial element in cluster
    next_element< T >& next_elem; // Functor to recursively generate elements in cluster

    int max_n;          // limit to number of elements
    int depth;          // counter to keep track of depth in tree
    int max_depth;      // prevent infinite recursion
    float min_scale;    // approximately one pixel

    std::optional< bb2f > bounds;

    // Recursively generate branches and render elements
    void render( image< T >& img, const float& t = 0.0f );

    // change root element parameters for branching cluster
    void set_root( element< T >& el );

    // render into a buffer pair. Rendering modifies image directly - does not require buffer swap.
    // in this case the cluster serves as an effect functor (the effect being rendering)
    void operator () ( buffer_pair< T >& buf, const float& t );

    cluster( const element< T >& el,  
             next_element< T >& next_elem_init, 
             const int& max_n_init = 100,
             const int& depth_init = 0,
             const int& max_depth_init = 10,
             const float& min_scale_init = 0.001f,
             const std::optional< bb2f >& bounds_init = std::nullopt
            )
        : root_elem( el ),
          next_elem( next_elem_init ),
          max_n( max_n_init ),
          depth( depth_init ),
          max_depth( max_depth_init ),
          min_scale( min_scale_init ),
          bounds( bounds_init ) 
          {}
};

/*
typedef std::variant<   std::unique_ptr< cluster< frgb > >, 
                        std::unique_ptr< cluster< ucolor > >, 
                        std::unique_ptr< cluster< vec2f > > 
                    > any_cluster;

class fimage;
class uimage;
class vector_field;

typedef std::variant<   std::unique_ptr< fimage >, 
                        std::unique_ptr< uimage >, 
                        std::unique_ptr< vector_field > 
                    > any_image;

struct scene {
    // global scene properties
    fimage background;
    // effect< frgb > eff;

    std::vector< any_cluster > clusters;
    std::map< std::string, any_image > images;
    
    void load( std::string filename );   // Load scene file (JSON)
    void load_background( std::string filename ) { background.load( filename ); }

    template< class T > void add_cluster( cluster< T >& cl ) { clusters.push_back( any_cluster( cl ) ); }

    void render( std::string filename, float t = 0.0f ) { 
        // Make copy of background image
        fimage out( background );
        // apply effect to background
        // eff( out );
        for( auto cl : clusters ) cl.render( out );
        out.write_jpg( filename, 100 );
    }
};
*/
#endif // __SCENE_HPP