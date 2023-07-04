#ifndef __LIFE_HPP 
#define __LIFE_HPP

#include "buffer_pair.hpp"
#include "vector_field.hpp"
#include "warp_field.hpp"
#include "joy_rand.hpp"
#include "any_rule.hpp"
#include "any_image.hpp"
#include "next_element.hpp"
 
// funky codes for funky_sort
#define FUNK_BORG  0xffffffffaa00aa00
#define FUNK_WEB_L 0xffff0000ffff0000
#define FUNK_WEB_R 0xffffffff00000000
#define FUNK_TEST_L 0xffffffffcccccccc
#define FUNK_TEST_R 0xffaaffaaffaaffaa

// future - move alpha block to CA
template< class T > struct CA {
   std::vector< T > neighbors, result; // vector of neighbors - size depends on neighborhood
   // Future - use for image targeting
   // std::optional< std::reference_wrapper< image< T >& > > target;
   // std::optional< std::reference_wrapper< warp_field& > > warper;  
   any_rule rule;   
   CA_neighborhood neighborhood;

   int ca_frame;  // ca_frame counter
   int x, y;    // position of cell in image

   harness< float > p;  // probability of cell running
   bool edge_block; // if true, cells on the edge of the image will not run
   bool alpha_block; // if true, cells will run with probability ( 1 - alpha ) / 255
   // future: image block?

   //void set_rule( any_rule rule );
   void operator () ( any_buffer_pair_ptr& buf, element_context& context );

   CA() :  
      neighborhood( HOOD_MOORE ), 
      ca_frame(0) {}
   CA( const any_rule& rule ) : rule( rule ), ca_frame(0) {}
};

//typedef CA< frgb > CA_frgb;
typedef CA< ucolor > CA_ucolor;
//typedef CA< vec2f > CA_vec2f;
//typedef CA< int > CA_int;
//typedef CA< vec2i > CA_vec2i;

template< class T > struct rule_identity {
   CA_neighborhood operator () ( element_context& context );   
   void operator () ( CA< T >& ca );
   
   rule_identity() {}
};

#define rule_identity_frgb rule_identity< frgb >
#define rule_identity_ucolor rule_identity< ucolor >
#define rule_identity_vec2f rule_identity< vec2f >
#define rule_identity_int rule_identity< int >
#define rule_identity_vec2i rule_identity< vec2i >

// Rule functor for Conway's Game of Life
template< class T > struct rule_life {
   harness< T > on, off;  // colors to represent on and off states

   CA_neighborhood operator () ( element_context& context );
   void operator () ( CA< T >& ca );
            

   rule_life( const T& on_init, const T& off_init ) : on( on_init ), off( off_init ) {}
   rule_life();
};

#define rule_life_frgb rule_life< frgb >
#define rule_life_ucolor rule_life< ucolor >
#define rule_life_vec2f rule_life< vec2f >
#define rule_life_int rule_life< int >
#define rule_life_vec2i rule_life< vec2i >

// Rule functor for diffusion
template< class T > struct rule_diffuse {
   std::uniform_int_distribution< int > rand_4;

   CA_neighborhood operator () ( element_context& context );
   void operator () ( CA< T >& ca );
            

   rule_diffuse( bool alpha_block_init = false ) :
      rand_4( std::uniform_int_distribution< int >( 0, 3 ) ) { }
};

#define rule_diffuse_frgb rule_diffuse< frgb >
#define rule_diffuse_ucolor rule_diffuse< ucolor >
#define rule_diffuse_vec2f rule_diffuse< vec2f >
#define rule_diffuse_int rule_diffuse< int >
#define rule_diffuse_vec2i rule_diffuse< vec2i >

// Rule functor for color sorting - rotate so that the brightest pixels are in a given direction
template< class T > struct rule_gravitate {
   std::uniform_int_distribution< int > rand_4;
   direction4 direction;

   CA_neighborhood operator () ( element_context& context );
   void operator () ( CA< T >& ca );
            

   rule_gravitate( direction4 direction_init = direction4::D4_DOWN, bool alpha_block_init = false ) :
      direction( direction_init ),
      rand_4( std::uniform_int_distribution< int >( 0, 3 ) ) { }
};

#define rule_gravitate_frgb   rule_gravitate< frgb >
#define rule_gravitate_ucolor rule_gravitate< ucolor >
#define rule_gravitate_vec2f  rule_gravitate< vec2f >
#define rule_gravitate_int    rule_gravitate< int >
#define rule_gravitate_vec2i  rule_gravitate< vec2i >

// Bug preserved in amber. A version of gravitate with a bug that causes it to rotate in the opposite direction.
// Rule functor for color sorting - rotate so that the brightest pixels are in a given direction
template< class T > struct rule_snow {
   direction4 direction;

   CA_neighborhood operator () ( element_context& context );
   void operator () ( CA< T >& ca );
            

   rule_snow( direction4 direction_init = direction4::D4_DOWN, bool alpha_block_init = false ) :
      direction( direction_init ) { }
};

#define rule_snow_frgb rule_snow< frgb >
#define rule_snow_ucolor rule_snow< ucolor >
#define rule_snow_vec2f rule_snow< vec2f >
#define rule_snow_int rule_snow< int >
#define rule_snow_vec2i rule_snow< vec2i >

// Rule functor for color sorting - rotate so that the brightest pixels are in a given direction
template< class T > struct rule_pixel_sort {
   direction8 direction;
   harness< int > max_diff; // Maximum difference between pixels to be sorted (Manhattan distance)

   CA_neighborhood operator () ( element_context& context );
   void operator () ( CA< T >& ca );
            
   rule_pixel_sort( direction8 direction_init = direction8::D8_DOWN, bool alpha_block_init = false, int max_diff_init = 300 ) :
      direction( direction_init ),
      max_diff( max_diff_init ) {}
};


#define rule_pixel_sort_frgb rule_pixel_sort< frgb >
#define rule_pixel_sort_ucolor rule_pixel_sort< ucolor >
#define rule_pixel_sort_vec2f rule_pixel_sort< vec2f >
#define rule_pixel_sort_int rule_pixel_sort< int >
#define rule_pixel_sort_vec2i rule_pixel_sort< vec2i >

template< class T > struct rule_funky_sort {
   direction8 direction;
   CA_neighborhood hood; // Variant of Margolus offset neighborhood
   harness< int > max_diff; // Maximum difference between pixels to be sorted (Manhattan distance)

   // How funky to make it (64 bit truth table comparing pairs in the Margolus neighborhood)
   // future: make harness< unsigned long long > 
   unsigned long long dafunk_l;  // Funky lookup table for first pair, rotated by direction  
   unsigned long long dafunk_r;  // Funky lookup table for second pair, rotated by direction
   unsigned long long dafunk_d;  // Funky lookup table for diagonal pair, rotated by direction

   CA_neighborhood operator () ( element_context& context );
   void operator () ( CA< T >& ca );
            
   rule_funky_sort(  unsigned long long dafunk_l_init = 0, 
                     unsigned long long dafunk_r_init = 0, 
                     unsigned long long dafunk_d_init = FUNK_BORG, 
                     int max_diff_init = 300,
                     direction8 direction_init = D8_DOWN, 
                     CA_neighborhood hood_init = HOOD_HOUR
                     ) :
      dafunk_l( dafunk_l_init ),
      dafunk_r( dafunk_r_init ),
      dafunk_d( dafunk_d_init ),
      direction( direction_init ),
      hood( hood_init ),
      max_diff( max_diff_init ) {}
};

#define rule_funky_sort_frgb rule_funky_sort< frgb >
#define rule_funky_sort_ucolor rule_funky_sort< ucolor >
#define rule_funky_sort_vec2f rule_funky_sort< vec2f >
#define rule_funky_sort_int rule_funky_sort< int >
#define rule_funky_sort_vec2i rule_funky_sort< vec2i >

#endif // __LIFE_HPP 
