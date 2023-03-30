#include "any_effect.hpp"
#include "effect.hpp"
#include "scene.hpp"
#include "life.hpp"

any_effect_fn::any_effect_fn() : name( "identity_effect_default" ) { 
    std::shared_ptr< eff_identity > f( new eff_identity );
    fn = std::ref( *f ); 
    fn_ptr = f; 
}