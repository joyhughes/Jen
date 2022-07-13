#ifndef __SCENE_HPP
#define __SCENE_HPP

#include <variant>
#include <map>
#include <optional>
#include "image.hpp"
#include "fimage.hpp"

//template< class T > struct effect;

template< class T > struct element {
    image< T >& img;
    //std::optional< effect< T >& > eff;
    
    vec2f position; 		    // coordinates of element center (relative to parent cluster)
    float scale; 			    // radius of element
    float rotation; 	        // rotation in degrees
    std::optional< T > tint;	// change the color of element
    image< T >* mask;
    int index;

    void render( image< T >& in ) { in.splat( position, scale, rotation, tint, img ); }

    element() : position( { 0.0f, 0.0f }), 
                scale( 1.0f ), 
                rotation( 0.0f ),
                index( 0 ) {}

    element(    image< T >& img_init, 
                const vec2f& position_init =  { 0.0f, 0.0f },
                const float& scale_init = 1.0f,
                const float& rotation_init = 0.0f,
                const std::optional< T >& tint_init = std::nullopt,
                image< T >* mask_init = NULL 
            ) : 
                img( img_init ),
                position( position_init ),
                scale( scale_init ),
                rotation( rotation_init ),
                tint( tint_init ),
                mask( mask_init ),
                index( 0 ) {}
};

template< class T > struct cluster;

template< class T > struct next_element;

template< class T > struct cluster {
    /* vec2f position;
    float scale;
    float rotation; */
    std::vector< std::unique_ptr< cluster< T > > > branches;

    element< T > root_elem;       // initial element in cluster
    next_element< T >& next_elem; // Functor to recursively generate elements in cluster

    int max_n;      // limit to number of elements
    int depth;      // counter to keep track of depth in tree
    int max_depth;  // prevent infinite recursion

    std::optional< bb2f > bounds;
    

/*
    void generate(  // generation function object , - fill the cluster based on its generator
                    float t = 0.0f // time
                    ); 

    // needs a different name
    void iterate(   // iteration function object , - evolve cluster based on current state
                    float step = 1.0f // timestep, speed, scaling factor
                    );
    */
    void render(    image< T >& in, float t = 0.0f );

    //cluster() : max_n( 100 ), depth( 0 ), max_depth( 10 ) {}
    cluster( const element< T >& el,  
             next_element< T >& next_elem_init, 
             const int& max_n_init = 100,
             const int& depth_init = 0,
             const int& max_depth_init = 10,
             const std::optional< bb2f >& bounds_init = std::nullopt
            )
        : root_elem( el ),
          next_elem( next_elem_init ),
          max_n( max_n_init ),
          depth( depth_init ),
          max_depth( max_depth_init ),
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