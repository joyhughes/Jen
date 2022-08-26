#include "life.hpp"
#include "frgb.hpp"
#include "ucolor.hpp"
#include "vect2.hpp"

template< class T > bool life< T >::operator () ( buffer_pair< T >& buf, const float& t ) { 
    if( buf.has_image() ) {
        // Use buffer_pair operator () to return reference to first member of pair
        auto in =  buf.get_image().begin();
        auto out = buf.get_buffer().begin();

        int xdim = buf.get_image().get_dim().x;
        int ydim = buf.get_image().get_dim().y;

        T white, black;
        ::white( white ); ::black( black );

        // initialize pointers for the upper left corner - wrap around (toroidal topolgy)
        auto ul = in + xdim * ydim - 1;      auto um = in + (ydim - 1) * xdim;    auto ur = in + (ydim - 1) * xdim + 1;
        auto ml = in + xdim - 1;             auto mm = in;                        auto mr = in + 1;
        auto dl = in + 2 * xdim - 1;         auto dm = in + xdim;                 auto dr = in + xdim + 1;

        auto result = out;

        for( int y=0; y<ydim; y++ ) {
        for( int x=0; x<xdim; x++ ) {

            // implement game of life rule 
            // ***************************
            
            int count = 0;
            if( *ul == white ) count++;  if( *um == white ) count++;  if( *ur == white ) count++; 
            if( *ml == white ) count++;                               if( *mr == white ) count++; 
            if( *dl == white ) count++;  if( *dm == white ) count++;  if( *dr == white ) count++;

            // count = *ul + *um + *ur + *ml + *mr + *dl + *dm + *dr;
            if( *mm == white )
            {
                // case where center cell is filled
                if(count == 2 | count == 3) *result = white; else *result = black;
            }
            else
            {
                // case where center cell is empty
                if(count == 3) *result = white; else *result = black;
            }   

            // ***************************
            // end rule implementation

            // increment iterators
            ul++; um++; ur++; ml++; mm++; mr++; dl++; dm++; dr++; result++;

            //deal with horizontal wrap 
            // pop left hand pointers to far left (previous row)
            if(x == 0) { ul -= xdim; ml -= xdim; dl -= xdim; }
            // move right hand pointers to far left
            if(x == xdim-2) { ur -= xdim; mr -= xdim; dr -= xdim; }
        }

        // move right and left colums down
        ul+=xdim; ml+=xdim; dl+=xdim; 
        ur+=xdim; mr+=xdim; dr+=xdim; 

        // deal with vertical wrap
        // pop upper pointers to upper row
        if(y == 0)      { ul -= xdim * ydim; um -= xdim * ydim; ur -= xdim * ydim; }

        // move lower pointers to upper row
        if(y == ydim-2) { dl -= xdim * ydim; dm -= xdim * ydim; dr -= xdim * ydim; }          
        }

        buf.swap();
        return true;
    }
    else {
        return false;
    }
}

template class life< frgb >;       // fimage
template class life< ucolor >;     // uimage
template class life< vec2f >;      // vector_field


