#ifndef __ANY_RULE_HPP
#define __ANY_RULE_HPP

#include <variant>
#include <vector>
#include <string>
#include <memory>
#include "ucolor.hpp"

typedef enum CA_hood  {  HOOD_MOORE, 
                                 // HOOD_VON_NEUMANN,
                                 // HOOD_VON_NEUMANN_DIAGONAL, 
                                 HOOD_MARGOLUS,
                                 HOOD_HOUR, // forward hourglass
                                 HOOD_HOUR_REV, // backward hourglass
                                 HOOD_BOW, // bowtie
                                 HOOD_BOW_REV, // backward bowtie
                                 HOOD_SQUARE, // square
                                 HOOD_SQUARE_REV, // backward square
                                 HOOD_RANDOM // ,   // random offset Margolus
                                 /* HOOD_HEXAGONAL,
                                 HOOD_HEXAGONAL_RANDOM,
                                 HOOD_TRIANGULAR,
                                 HOOD_TRIANGULAR_RANDOM,
                                 HOOD_PENTAGONAL,
                                 HOOD_PENTAGONAL_RANDOM,
                                 HOOD_HEXADECAGONAL,
                                 HOOD_HEXADECAGONAL_RANDOM,
                                 HOOD_OCTAGONAL,
                                 HOOD_OCTAGONAL_RANDOM,
                                 HOOD_STAR,
                                 HOOD_STAR_RANDOM,
                                 HOOD_CUSTOM,
                                 HOOD_CUSTOM_RANDOM */
} CA_hood;

struct element_context;

template< class T > struct CA;

template < class T > struct rule_identity;
template < class T > struct rule_life;
template < class T > struct rule_random_copy;
template < class T > struct rule_random_mix;
template < class T > struct rule_diffuse;
template < class T > struct rule_gravitate;
template < class T > struct rule_snow;
template < class T > struct rule_pixel_sort;
template < class T > struct rule_funky_sort;

typedef std::variant <
    std::shared_ptr< rule_identity< ucolor > >,
    std::shared_ptr< rule_life< ucolor > >,
    std::shared_ptr< rule_random_copy< ucolor > >,
    std::shared_ptr< rule_random_mix< ucolor > >,
    std::shared_ptr< rule_diffuse< ucolor > >,
    std::shared_ptr< rule_gravitate< ucolor > >,
    std::shared_ptr< rule_snow< ucolor > >,
    std::shared_ptr< rule_pixel_sort< ucolor > >,
    std::shared_ptr< rule_funky_sort< ucolor > >
> any_rule_ptr;

// only works for ucolor at this point
struct any_rule {
    typedef std::function< void ( CA< ucolor >& ) > CA_rule;
    typedef std::function< CA_hood ( element_context& ) > CA_initializer;

    any_rule_ptr rule_ptr;
    CA_rule rule;
    CA_initializer initializer;
    std::string name;

    void operator () ( CA< ucolor >& ca );  // call the rule
    CA_hood init( element_context& context );  // call the initializer

    any_rule();
    any_rule( any_rule_ptr rule_ptr, CA_rule rule, CA_initializer initializer, std::string name ) 
    : rule_ptr( rule_ptr ), rule( rule ), initializer( initializer ), name( name ) {}
};

#endif // __ANY_RULE_HPP