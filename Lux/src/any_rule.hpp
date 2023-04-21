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
    typedef std::function< void ( std::vector< ucolor >&, std::vector< ucolor >& ) > CA_rule;
    any_rule_ptr rule_ptr;
    CA_rule rule;
    CA_neighborhood neighborhood;
    std::string name;

    void operator () ( std::vector< ucolor >& in, std::vector< ucolor >& out );

    any_rule();
    any_rule( any_rule_ptr rule_ptr, CA_rule rule, CA_neighborhood neighborhood, std::string name ) 
    : rule_ptr( rule_ptr ), rule( rule ), neighborhood( neighborhood ), name( name ) {}
};

#endif // __ANY_RULE_HPP