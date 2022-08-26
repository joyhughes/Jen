#include "image.hpp"

// future: margolis neighborhood

// Component effect - wrapper for warp with vector field
template< class T > struct life {

   // In this case t has no effect
   bool operator () ( buffer_pair< T >& buf, const float& t = 0.0f );

};



