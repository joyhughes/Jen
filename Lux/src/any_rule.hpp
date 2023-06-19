#ifndef __ANY_RULE_HPP
#define __ANY_RULE_HPP

#include <variant>
#include <vector>
#include <string>
#include <memory>
#include "ucolor.hpp"

typedef enum CA_neighborhood  {  NEIGHBORHOOD_MOORE, 
                                 NEIGHBORHOOD_VON_NEUMANN,
                                 NEIGHBORHOOD_VON_NEUMANN_DIAGONAL, 
                                 NEIGHBORHOOD_MARGOLUS,
                                 NEIGHBORHOOD_MARGOLUS_OFFSET,
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

struct element_context;

template< class T > struct CA;

template < class T > struct rule_identity;
template < class T > struct rule_life;
template < class T > struct rule_diffuse;
template < class T > struct rule_gravitate;
template < class T > struct rule_snow;
template < class T > struct rule_pixel_sort;

typedef std::variant <
    std::shared_ptr< rule_identity< ucolor > >,
    std::shared_ptr< rule_life< ucolor > >,
    std::shared_ptr< rule_diffuse< ucolor > >,
    std::shared_ptr< rule_gravitate< ucolor > >,
    std::shared_ptr< rule_snow< ucolor > >,
    std::shared_ptr< rule_pixel_sort< ucolor > >
> any_rule_ptr;


// only works for ucolor at this point
struct any_rule {
    typedef std::function< void ( CA< ucolor >& ) > CA_rule;
    typedef std::function< CA_neighborhood ( element_context& ) > CA_initializer;

    any_rule_ptr rule_ptr;
    CA_rule rule;
    CA_initializer initializer;
    std::string name;

    void operator () ( CA< ucolor >& ca );  // call the rule
    CA_neighborhood init( element_context& context );  // call the initializer

    any_rule();
    any_rule( any_rule_ptr rule_ptr, CA_rule rule, CA_initializer initializer, std::string name ) 
    : rule_ptr( rule_ptr ), rule( rule ), initializer( initializer ), name( name ) {}
};

#endif // __ANY_RULE_HPP