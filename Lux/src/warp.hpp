#ifndef __WARP_HPP
#define __WARP_HPP

#include "effect.hpp"
#include "vector_field.hpp"

// Melts by iteratively running effect function
// Remembers its current state- owning buffer pair
template< class T > struct iterative_melt {
    typedef std::function< image< T >& () > image_fn;
    typedef std::function< bool ( buffer_pair< T >&, const float& ) > eff_fn;

    eff_fn   eff;
    buffer_pair< T > buf;    // owns this buffer pair
    int frame;

    void iterate( const float& t = 0.0f ) { eff( buf, t ); frame++; }
    image< T >& operator () () { return buf(); }
    // void reset();

    iterative_melt( const eff_fn& eff_init, const image< T >& img ) : eff( eff_init ), buf( img ), frame( 0 ) {}
};

// Melts by repeatedly warping using a vector field
// Remembers its current state- accumulated vector field
template< class T > struct vector_melt {
    typedef std::function< image< T >& () > image_fn;
    vector_field& driver;

    vector_field vf;    // integrated vector warp
    int frame;
    float step;

    void iterate( const float& t = 0.0f ) {
         // run function for each point in vector field and integrate
        // std::cout << "vector_melt dim.x  " << vf.get_dim().x << "\n";
        auto vfit = vf.begin(); 
        vec2f coord;
        for( int y = 0; y < vf.get_dim().y; y++ ) {
            for( int x = 0; x < vf.get_dim().x; x++ ) {
                coord = vf.get_bounds().template bb_map< vec2f, 2 >( { x, y }, vf.get_ipbounds() );
                *vfit += step * driver.sample( coord + *vfit, true, SAMP_REPEAT );
                vfit++;
            }
        } 
        frame++; 
    }

    // apply accumulated warp to any image
    bool operator () ( buffer_pair< T >& buf, const float& t = 0.0f ) { 
        // create warp function using integrated vector field
        eff_vector_warp< T > vwarp( vf, 1.0f, true, true, SAMP_REPEAT );
        return vwarp( buf, t );
    }

    // void reset();

    vector_melt( vector_field& driver_init, const float& step_init ) : driver( driver_init ), vf( driver.get_dim() ), step( step_init ), frame( 0 ) {}
};

// Melts by repeatedly warping using a vector function
// Remembers its current state- accumulated vector field
template< class T > struct functional_melt {
    typedef std::function< image< T >& () > image_fn;

    vector_field vf;    // integrated vector warp
    vector_fn vfn;
    float step;
    int frame;

    void iterate( const float& t = 0.0f ) { 
         // run function for each point in vector field and integrate
        auto vfit = vf.begin(); 
        vec2f coord;
        for( int y = 0; y < vf.get_dim().y; y++ ) {
            for( int x = 0; x < vf.get_dim().x; x++ ) {
                coord = vf.get_bounds().template bb_map< vec2f, 2 >( { x, y }, vf.get_ipbounds() );
                *vfit += step * vfn( coord + *vfit, t );
                vfit++;
            }
        } 
        frame++; 
    }

    bool operator () ( buffer_pair< T >& buf, const float& t = 0.0f ) { 
        // create warp function using integrated vector field
        eff_vector_warp< T > vwarp( vf, 1.0f, true, true, SAMP_REPEAT );
        return vwarp( buf, t );
    }

    // void reset();
    functional_melt( const vector_fn& vfn_init, const float &step_init = 1.0f, vec2i dim = { 1000, 1000 } ) : vfn( vfn_init ), step( step_init ), vf( dim ), frame( 0 ) {}
};

#endif // __WARP_HPP
