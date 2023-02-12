#include "image.hpp"
#include "vector_field.hpp"
#include "warp_field.hpp"
#include "joy_rand.hpp"

typedef enum direction4 { UP, RIGHT, DOWN, LEFT } direction4; // clockwise
//typedef enum direction8 { UP, UPRIGHT, RIGHT, DOWNRIGHT, DOWN, DOWNLEFT, LEFT, UPLEFT } direction8;

typedef enum CA_neighborhood  {  NEIGHBORHOOD_MOORE, 
                                 NEIGHBORHOOD_VON_NEUMANN,
                                 NEIGHBORHOOD_VON_NEUMANN_DIAGONAL, 
                                 NEIGHBORHOOD_MARGOLIS,
                                 NEIGHBORHOOD_HEXAGONAL,
                                 NEIGHBORHOOD_HEXAGONAL_RANDOM,
                                 NEIGHBORHOOD_TRIANGULAR,
                                 NEIGHBORHOOD_TRIANGULAR_RANDOM,
                                 NEIGHBORHOOD_PENTAGONAL,
                                 NEIGHBORHOOD_PENTAGONAL_RANDOM,
                                 NEIGHBORHOOD_HEXADECAGONAL,
                                 NEIGHBORHOOD_HEXADECAGONAL_RANDOM,
                                 NEIGHBORHOOD_OCTAGONAL,
                                 NEIGHBORHOOD_OCTAGONAL_RANDOM,
                                 NEIGHBORHOOD_STAR,
                                 NEIGHBORHOOD_STAR_RANDOM,
                                 NEIGHBORHOOD_CUSTOM,
                                 NEIGHBORHOOD_CUSTOM_RANDOM
} CA_neighborhood;
 
typedef enum mutation_type {  MUTATE_NONE, 
                              MUTATE_COLOR, 
                              MUTATE_HUE,
                              MUTATE_VALUE,
                              MUTATE_SATURATION 
} mutation_type;

template< class T > struct CA {
   typedef std::function< void ( std::vector< T >&, std::vector< T >& ) > CA_rule;

   std::vector< T > neighbors, result;
   // Future - use for image targeting
   // std::optional< std::reference_wrapper< image< T >& > > target;
   // std::optional< std::reference_wrapper< warp_field& > > warper;  
   CA_rule rule;   // vector of neighbors - size depends on neighborhood
   CA_neighborhood neighborhood;
   mutation_type mutate_type;
   float mutate_amount;
   float mutate_probability;
   int frame;  // frame counter

   void set_rule( CA_rule rule, CA_neighborhood neighborhood ) { this->rule = rule; this->neighborhood = neighborhood; }
   void operator () ( buffer_pair< T >& buf, const float& t = 0.0f ); // iterates CA

   CA( CA_rule rule, CA_neighborhood neighborhood, mutation_type mutate_type = MUTATE_NONE, float mutate_amount = 0.0f, float mutate_probability = 0.0f ) : rule( rule ), neighborhood( neighborhood ), mutate_type( mutate_type ), mutate_amount( mutate_amount ), mutate_probability( mutate_probability ), frame(0) {}
};

// Rule functor for Conway's Game of Life
template< class T > struct life {
   const CA_neighborhood neighborhood;
   T on, off;  // colors to represent on and off states

   void operator () ( const std::vector< T >& neighbors, std::vector< T >& result );

   life( const T& on_init, const T& off_init ) : on( on_init ), off( off_init ), neighborhood( NEIGHBORHOOD_MOORE ) {}
   life() : neighborhood( NEIGHBORHOOD_MOORE ) { white( on ); black( off ); }
};

// Rule functor for diffusion
template< class T > struct diffuse {
   const CA_neighborhood neighborhood;
   std::uniform_int_distribution< int > rand_4;
   bool alpha_block; // any neighborhood containing a pixel with alpha != 0 will not diffuse 

   void operator () ( const std::vector< T >& neighbors, std::vector< T >& result );

   diffuse( bool alpha_block_init = false ) :
      neighborhood( NEIGHBORHOOD_MARGOLIS ), 
      rand_4( std::uniform_int_distribution< int >( 0, 3 ) ),
      alpha_block( alpha_block_init) {}
};

// Rule functor for color sorting - rotate so that the brightest pixels are in a given direction
template< class T > struct gravitate {
   const CA_neighborhood neighborhood;
   std::uniform_int_distribution< int > rand_4;
   direction4 direction;
   bool alpha_block; // any neighborhood containing a pixel with alpha != 0 will not diffuse 

   void operator () ( const std::vector< T >& neighbors, std::vector< T >& result );

   gravitate( direction4 direction_init = DOWN, bool alpha_block_init = false ) :
      direction( direction_init ),
      neighborhood( NEIGHBORHOOD_MARGOLIS ), 
      rand_4( std::uniform_int_distribution< int >( 0, 3 ) ),
      alpha_block( alpha_block_init) {}
};

// Bug preserved in amber. A version of gravitate with a bug that causes it to rotate in the opposite direction.
// Rule functor for color sorting - rotate so that the brightest pixels are in a given direction
template< class T > struct snow {
   const CA_neighborhood neighborhood;
   std::uniform_int_distribution< int > rand_4;
   direction4 direction;
   bool alpha_block; // any neighborhood containing a pixel with alpha != 0 will not diffuse 

   void operator () ( const std::vector< T >& neighbors, std::vector< T >& result );

   snow( direction4 direction_init = DOWN, bool alpha_block_init = false ) :
      direction( direction_init ),
      neighborhood( NEIGHBORHOOD_MARGOLIS ), 
      rand_4( std::uniform_int_distribution< int >( 0, 3 ) ),
      alpha_block( alpha_block_init) {}
};

// Rule functor for color sorting - rotate so that the brightest pixels are in a given direction
template< class T > struct pixel_sort {
   const CA_neighborhood neighborhood;
   std::uniform_int_distribution< int > rand_4;
   direction4 direction;
   bool alpha_block; // any neighborhood containing a pixel with alpha != 0 will not diffuse 
   int max_diff; // Maximum difference between pixels to be sorted (Manhattan distance)

   void operator () ( const std::vector< T >& neighbors, std::vector< T >& result );

   pixel_sort( direction4 direction_init = DOWN, bool alpha_block_init = false, int max_diff_init = 300 ) :
      direction( direction_init ),
      neighborhood( NEIGHBORHOOD_MARGOLIS ), 
      rand_4( std::uniform_int_distribution< int >( 0, 3 ) ),
      alpha_block( alpha_block_init),
      max_diff( max_diff_init ) {}
};
