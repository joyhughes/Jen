#ifndef __ANY_RULE_HPP
#define __ANY_RULE_HPP

#include <variant>
#include <vector>
#include <string>
#include <memory>
#include "ucolor.hpp"

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
    std::string name;

    void operator () ( std::vector< ucolor >& in, std::vector< ucolor >& out );

    any_rule();
    any_rule( any_rule_ptr rule_ptr, CA_rule rule, std::string name ) : rule_ptr( rule_ptr ), rule( rule ), name( name ) {}
};

#endif // __ANY_RULE_HPP