#include "any_rule.hpp"
#include "life.hpp"

any_rule::any_rule() : name( "identity_rule_default" ) { 
    std::shared_ptr< rule_identity< ucolor > > f( new rule_identity< ucolor > );
    rule = std::ref( *f ); 
    rule_ptr = f; 
}

void any_rule::operator () ( std::vector< ucolor >& in, std::vector< ucolor >& out ) {
    rule( in, out );
}
