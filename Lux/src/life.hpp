#ifndef __LIFE_HPP 
#define __LIFE_HPP

#include "buffer_pair.hpp"
#include "vector_field.hpp"
#include "warp_field.hpp"
#include "joy_rand.hpp"
#include "any_rule.hpp"
#include "any_image.hpp"
#include "life_hacks.hpp"
#include "next_element.hpp"
#include <array>
 
// collected funky codes for funky_sort
#define FUNK_BORG   0xffffffffaa00aa00
#define FUNK_WEB_L  0xffff0000ffff0000
#define FUNK_WEB_R  0xffffffff00000000
#define FUNK_TEST_L 0xffffffffcccccccc
#define FUNK_TEST_R 0xffaaffaaffaaffaa

// future - move alpha block to CA
template< class T > struct CA {
   std::vector< T > neighbors, result, targ; // vector of neighbors - size depends on neighborhood
   // Future - use for image targeting
   bool targeted; // if true, rule will run if it gets closer to target image
   any_buffer_pair_ptr target; // target image
   // 
   // Rule can have both warp field and vector field
   // For choice between fast and smooth
   // std::optional< std::reference_wrapper< warp_field& > > wf; 
   // std::optional< std::reference_wrapper< vector_field& > > vf; 
   // std::optional< std::reference_wrapper< warp_field& > > rule_map; 

   any_rule rule;   // Multiple rules?
   // std::vector< any_rule > rules; // table of rules for rule map

   CA_hood hood;

   // relevant information
   int ca_frame;  // ca_frame counter
   int x, y;    // position of cell in image
   vec2i dim;   // dimensions of image

   // Built in conditions
   harness< float > p;  // probability of cell running
   harness< bool > edge_block; // if true, cells on the edge of the image will not run
   harness< bool > alpha_block; // if true, cells will run with probability ( 1 - alpha ) / 255
   harness< bool > bright_block;
   harness< interval_int > bright_range; // cells with brightness within range will run
   // future: image block

   //void set_rule( any_rule rule );
   void run_rule();
   void operator () ( any_buffer_pair_ptr& buf, element_context& context );

   CA() :  // default constructor for rule returns identity rule pointer
      rule(),
      targeted( false ),
      target( null_buffer_pair_ptr ),
      hood( HOOD_MOORE ), 
      p( 1.0f ),
      edge_block( false ),
      alpha_block( false ),
      bright_block( false ),
      bright_range( { 0, 768 } ),
      ca_frame(0) {}
};

//typedef CA< frgb > CA_frgb;
typedef CA< ucolor > CA_ucolor;
//typedef CA< vec2f > CA_vec2f;
//typedef CA< int > CA_int;
//typedef CA< vec2i > CA_vec2i;

template< class T > struct rule_identity {
   CA_hood operator () ( element_context& context );   
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
   bool use_threshold; // if true, use thresholding to determine on and off states
   harness< int > threshold; // threshold value

   CA_hood operator () ( element_context& context );
   void operator () ( CA< T >& ca );
            
   rule_life( const T& on_init = white< T >, const T& off_init = black< T >, const bool& use_threshold_init = false, const int& threshold_init = 384 ) : on( on_init ), off( off_init ), use_threshold( use_threshold_init ), threshold( threshold_init ) {}
};

#define rule_life_frgb rule_life< frgb >
#define rule_life_ucolor rule_life< ucolor >
#define rule_life_vec2f rule_life< vec2f >
#define rule_life_int rule_life< int >
#define rule_life_vec2i rule_life< vec2i >

// Randomly copies color from a random neighbor (self included)
template< class T > struct rule_random_copy {
   std::uniform_int_distribution< int > rand_9;

   CA_hood operator () ( element_context& context );
   void operator () ( CA< T >& ca );

   rule_random_copy() : rand_9( std::uniform_int_distribution< int >( 0, 8 ) ) {}
};

#define rule_random_copy_frgb rule_random_copy< frgb >
#define rule_random_copy_ucolor rule_random_copy< ucolor >
#define rule_random_copy_vec2f rule_random_copy< vec2f >
#define rule_random_copy_int rule_random_copy< int >
#define rule_random_copy_vec2i rule_random_copy< vec2i >

// Randomly mixes color components of neighbors, including self
template< class T > struct rule_random_mix {
   std::uniform_int_distribution< int > rand_9;

   CA_hood operator () ( element_context& context );
   void operator () ( CA< T >& ca );

   rule_random_mix() : rand_9( std::uniform_int_distribution< int >( 0, 8 ) ) {}
};

#define rule_random_mix_frgb rule_random_mix< frgb >
#define rule_random_mix_ucolor rule_random_mix< ucolor >
#define rule_random_mix_vec2f rule_random_mix< vec2f >
#define rule_random_mix_int rule_random_mix< int >
#define rule_random_mix_vec2i rule_random_mix< vec2i >

template< class T > struct rule_box_blur {
   harness< int > max_diff; // Maximum difference between pixels to be blurred (Manhattan distance)
   harness< box_blur_type > blur_method; // Type of box blur
   harness< bool > bug_mode; // if true, return default color if no neighbors are within max_diff
   harness< bool > random_copy; // if true, randomly copy color from a neighbor instead of blurring
   std::array< unsigned int, 8 > diffs; // Manhattan distance to each neighbor

   //harness< int > dirs_size; // number of directions
   //std::vector< harness < int > > dirs;  // array of directions for blur
   //std::vector< harness < int > > compares; // array of directions for comparison with max_diff
   std::string custom_picker; // name of custom blur picker
   std::vector< int > dirs;  // array of directions for blur
   std::vector< int > compares; // array of directions for comparison with max_diff

   CA_hood operator () ( element_context& context );
   void operator () ( CA< T >& ca );

   rule_box_blur( int max_diff_init = 230, 
                  box_blur_type blur_method_init = BB_ORTHOGONAL, 
                  bool bug_mode_init = true ) : 
      max_diff( max_diff_init ),
      blur_method( blur_method_init ),
      bug_mode( bug_mode_init ) {}
};

#define rule_box_blur_frgb rule_box_blur< frgb >
#define rule_box_blur_ucolor rule_box_blur< ucolor >
#define rule_box_blur_vec2f rule_box_blur< vec2f >
#define rule_box_blur_int rule_box_blur< int >
#define rule_box_blur_vec2i rule_box_blur< vec2i >

// Colors drift separately in given directions
// Alternative - continuous direction function, use monte carlo interpolation
/*
template< class T > struct rule_color_drift {
   std::uniform_int_distribution< int > rand_9;
   harness< int > drift_r, drift_g, drift_b; // integer 0 to 9

   CA_hood operator () ( element_context& context );
   void operator () ( CA< T >& ca );

   rule_random_mix( int drift_r_init, int drift_g_init, int drift_b_init ) : 
      rand_9( std::uniform_int_distribution< int >( 0, 8 ) ),
      drift_r( drift_r_init % 9 ),
      drift_g( drift_g_init % 9 ),
      drift_b( drift_b_init % 9 ) {}
};

#define rule_color_drift_frgb rule_color_drift< frgb >
#define rule_color_drift_ucolor rule_color_drift< ucolor >
#define rule_color_drift_vec2f rule_color_drift< vec2f >
#define rule_color_drift_int rule_color_drift< int >
#define rule_color_drift_vec2i rule_color_drift< vec2i >
*/

// Margolus neighborhood

// Rule functor for diffusion
template< class T > struct rule_diffuse {
   std::uniform_int_distribution< int > rand_4;

   CA_hood operator () ( element_context& context );
   void operator () ( CA< T >& ca );
            

   rule_diffuse() :
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
   harness< direction4 > direction;

   CA_hood operator () ( element_context& context );
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
   harness< direction4 > direction;

   CA_hood operator () ( element_context& context );
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
   harness< direction8 > direction;
   harness< int > max_diff; // Maximum difference between pixels to be sorted (Manhattan distance)

   CA_hood operator () ( element_context& context );
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
   harness< direction4 > direction;
   CA_hood hood; // Variant of Margolus offset neighborhood
   harness< int > max_diff; // Maximum difference between pixels to be sorted (Manhattan distance)

   // How funky to make it (64 bit truth table comparing pairs in the Margolus neighborhood)
   // future: make harness< unsigned long long > 
   harness< funk_factor > dafunk_l;  // Funky lookup table for first pair, rotated by direction  
   harness< funk_factor > dafunk_r;  // Funky lookup table for second pair, rotated by direction

   CA_hood operator () ( element_context& context );
   void operator () ( CA< T >& ca );
            
   rule_funky_sort(  funk_factor dafunk_l_init = FUNK_WEB_L, 
                     funk_factor dafunk_r_init = FUNK_WEB_R, 
                     int max_diff_init = 300,
                     direction4 direction_init = D4_DOWN, 
                     CA_hood hood_init = HOOD_HOUR
                     ) :
      dafunk_l( dafunk_l_init ),
      dafunk_r( dafunk_r_init ),
      direction( direction_init ),
      hood( hood_init ),
      max_diff( max_diff_init ) {}
};

#define rule_funky_sort_frgb rule_funky_sort< frgb >
#define rule_funky_sort_ucolor rule_funky_sort< ucolor >
#define rule_funky_sort_vec2f rule_funky_sort< vec2f >
#define rule_funky_sort_int rule_funky_sort< int >
#define rule_funky_sort_vec2i rule_funky_sort< vec2i >

template< class T > struct rule_diagonal_funky_sort {
   harness< direction4_diagonal > direction;
   CA_hood hood; // Variant of Margolus offset neighborhood
   harness< int > max_diff; // Maximum difference between pixels to be sorted (Manhattan distance)

   // How funky to make it (64 bit truth table comparing pairs in the Margolus neighborhood)
   harness< funk_factor > dafunk_d;  // Funky lookup table for diagonal pair, rotated by direction

   CA_hood operator () ( element_context& context );
   void operator () ( CA< T >& ca );
            
   rule_diagonal_funky_sort( funk_factor dafunk_d_init = FUNK_BORG, 
                           int max_diff_init = 300,
                           direction4_diagonal direction_init = D4D_DOWNRIGHT, 
                           CA_hood hood_init = HOOD_HOUR
                           ) :
      dafunk_d( dafunk_d_init ),
      direction( direction_init ),
      hood( hood_init ),
      max_diff( max_diff_init ) {}
};

#define rule_diagonal_funky_sort_frgb rule_diagonal_funky_sort< frgb >
#define rule_diagonal_funky_sort_ucolor rule_diagonal_funky_sort< ucolor >
#define rule_diagonal_funky_sort_vec2f rule_diagonal_funky_sort< vec2f >
#define rule_diagonal_funky_sort_int rule_diagonal_funky_sort< int >
#define rule_diagonal_funky_sort_vec2i rule_diagonal_funky_sort< vec2i >

#endif // __LIFE_HPP 