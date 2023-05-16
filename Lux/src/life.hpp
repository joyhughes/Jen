#ifndef __LIFE_HPP 
#define __LIFE_HPP

#include "image.hpp"
#include "vector_field.hpp"
#include "warp_field.hpp"
#include "joy_rand.hpp"
#include "any_rule.hpp"
#include "any_image.hpp"
#include "next_element.hpp"
 
typedef enum mutation_type {  MUTATE_NONE, 
                              MUTATE_COLOR, 
                              MUTATE_HUE,
                              MUTATE_VALUE,
                              MUTATE_SATURATION 
} mutation_type;

template< class T > struct rule_identity {
   const CA_neighborhood neighborhood;

   void operator () ( const std::vector< T >& neighbors, std::vector< T >& result ) { result = neighbors; }

   rule_identity() : neighborhood( NEIGHBORHOOD_MARGOLIS ){}
};

#define rule_identity_frgb rule_identity< frgb >
#define rule_identity_ucolor rule_identity< ucolor >
#define rule_identity_vec2f rule_identity< vec2f >
#define rule_identity_int rule_identity< int >
#define rule_identity_vec2i rule_identity< vec2i >

// Rule functor for Conway's Game of Life
template< class T > struct rule_life {
   const CA_neighborhood neighborhood;
   T on, off;  // colors to represent on and off states

   void operator () ( const std::vector< T >& neighbors, std::vector< T >& result );

   rule_life( const T& on_init, const T& off_init ) : on( on_init ), off( off_init ), neighborhood( NEIGHBORHOOD_MOORE ) {}
   rule_life() : neighborhood( NEIGHBORHOOD_MOORE ) { white( on ); black( off ); }
};

#define rule_life_frgb rule_life< frgb >
#define rule_life_ucolor rule_life< ucolor >
#define rule_life_vec2f rule_life< vec2f >
#define rule_life_int rule_life< int >
#define rule_life_vec2i rule_life< vec2i >

// Rule functor for diffusion
template< class T > struct rule_diffuse {
   const CA_neighborhood neighborhood;
   std::uniform_int_distribution< int > rand_4;
   bool alpha_block; // any neighborhood containing a pixel with alpha != 0 will not diffuse 

   void operator () ( const std::vector< T >& neighbors, std::vector< T >& result );

   rule_diffuse( bool alpha_block_init = false ) :
      neighborhood( NEIGHBORHOOD_MARGOLIS ), 
      rand_4( std::uniform_int_distribution< int >( 0, 3 ) ),
      alpha_block( alpha_block_init) {}
};

#define rule_diffuse_frgb rule_diffuse< frgb >
#define rule_diffuse_ucolor rule_diffuse< ucolor >
#define rule_diffuse_vec2f rule_diffuse< vec2f >
#define rule_diffuse_int rule_diffuse< int >
#define rule_diffuse_vec2i rule_diffuse< vec2i >

// Rule functor for color sorting - rotate so that the brightest pixels are in a given direction
template< class T > struct rule_gravitate {
   const CA_neighborhood neighborhood;
   std::uniform_int_distribution< int > rand_4;
   direction4 direction;
   bool alpha_block; // any neighborhood containing a pixel with alpha != 0 will not diffuse 

   void operator () ( const std::vector< T >& neighbors, std::vector< T >& result );

   rule_gravitate( direction4 direction_init = direction4::D4_DOWN, bool alpha_block_init = false ) :
      direction( direction_init ),
      neighborhood( NEIGHBORHOOD_MARGOLIS ), 
      rand_4( std::uniform_int_distribution< int >( 0, 3 ) ),
      alpha_block( alpha_block_init) {}
};

#define rule_gravitate_frgb rule_gravitate< frgb >
#define rule_gravitate_ucolor rule_gravitate< ucolor >
#define rule_gravitate_vec2f rule_gravitate< vec2f >
#define rule_gravitate_int rule_gravitate< int >
#define rule_gravitate_vec2i rule_gravitate< vec2i >

// Bug preserved in amber. A version of gravitate with a bug that causes it to rotate in the opposite direction.
// Rule functor for color sorting - rotate so that the brightest pixels are in a given direction
template< class T > struct rule_snow {
   const CA_neighborhood neighborhood;
   std::uniform_int_distribution< int > rand_4;
   direction4 direction;
   bool alpha_block; // any neighborhood containing a pixel with alpha != 0 will not diffuse 

   void operator () ( const std::vector< T >& neighbors, std::vector< T >& result );

   rule_snow( direction4 direction_init = direction4::D4_DOWN, bool alpha_block_init = false ) :
      direction( direction_init ),
      neighborhood( NEIGHBORHOOD_MARGOLIS ), 
      rand_4( std::uniform_int_distribution< int >( 0, 3 ) ),
      alpha_block( alpha_block_init) {}
};

#define rule_snow_frgb rule_snow< frgb >
#define rule_snow_ucolor rule_snow< ucolor >
#define rule_snow_vec2f rule_snow< vec2f >
#define rule_snow_int rule_snow< int >
#define rule_snow_vec2i rule_snow< vec2i >

// Rule functor for color sorting - rotate so that the brightest pixels are in a given direction
template< class T > struct rule_pixel_sort {
   const CA_neighborhood neighborhood;
   std::uniform_int_distribution< int > rand_4;
   direction4 direction;
   bool alpha_block; // any neighborhood containing a pixel with alpha != 0 will not diffuse 
   harness< int > max_diff; // Maximum difference between pixels to be sorted (Manhattan distance)

   void operator () ( const std::vector< T >& neighbors, std::vector< T >& result );

   rule_pixel_sort( direction4 direction_init = direction4::D4_DOWN, bool alpha_block_init = false, int max_diff_init = 300 ) :
      direction( direction_init ),
      neighborhood( NEIGHBORHOOD_MARGOLIS ), 
      rand_4( std::uniform_int_distribution< int >( 0, 3 ) ),
      alpha_block( alpha_block_init),
      max_diff( max_diff_init ) {}
};

#define rule_pixel_sort_frgb rule_pixel_sort< frgb >
#define rule_pixel_sort_ucolor rule_pixel_sort< ucolor >
#define rule_pixel_sort_vec2f rule_pixel_sort< vec2f >
#define rule_pixel_sort_int rule_pixel_sort< int >
#define rule_pixel_sort_vec2i rule_pixel_sort< vec2i >

// future - move alpha block to CA
template< class T > struct CA {
   std::vector< T > neighbors, result; // vector of neighbors - size depends on neighborhood
   // Future - use for image targeting
   // std::optional< std::reference_wrapper< image< T >& > > target;
   // std::optional< std::reference_wrapper< warp_field& > > warper;  
   any_rule rule;   
   CA_neighborhood neighborhood;
   mutation_type mutate_type;
   harness< int > mutate_amount;
   harness< float > mutate_probability;
   harness< float > temperature; // probability rule will run even if result diverges from target
   int frame;  // frame counter

   void set_rule( any_rule rule );
   void operator () ( any_buffer_pair_ptr& buf, element_context& context );

   CA() :  
      neighborhood( NEIGHBORHOOD_MARGOLIS ), 
      mutate_type( MUTATE_NONE ),
      mutate_amount( 0 ), 
      mutate_probability( 0.0f ), 
      temperature( 0.0f ), 
      frame(0) {}
   CA( const any_rule& rule, 
   const CA_neighborhood& neighborhood, 
   const mutation_type& mutate_type = MUTATE_NONE, 
   const float& mutate_amount = 0.0f, 
   const float& mutate_probability = 0.0f, 
   const float& temperature = 0.0f ) : 
      rule( rule ), 
      neighborhood( neighborhood ), 
      mutate_type( mutate_type ), 
      mutate_amount( mutate_amount ), 
      mutate_probability( mutate_probability ), 
      temperature( temperature ),
      frame(0) {}
};

//typedef CA< frgb > CA_frgb;
typedef CA< ucolor > CA_ucolor;
//typedef CA< vec2f > CA_vec2f;
//typedef CA< int > CA_int;
//typedef CA< vec2i > CA_vec2i;

#endif // __LIFE_HPP
