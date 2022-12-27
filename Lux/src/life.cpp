#include "life.hpp"
#include "frgb.hpp"
#include "ucolor.hpp"
#include "vect2.hpp"

// Moore neighborhood shortcuts
#define UL neighbors[0]
#define UM neighbors[1]
#define UR neighbors[2]
#define ML neighbors[3]
#define MM neighbors[4]
#define MR neighbors[5]
#define DL neighbors[6]
#define DM neighbors[7]
#define DR neighbors[8]

// future - implement multiresolution rule on mip-map

// Uses toroidal boundary conditions
template< class T > bool CA< T >::operator() ( buffer_pair<T> &buf, const float &t ) {
    if(!buf.has_image()) return false;
    auto in =  buf.get_image().begin();
    auto out = buf.get_buffer().begin();
    vec2i dim = buf.get_image().get_dim();

    unsigned int on  = 0xffffffff;
    unsigned int off = 0xff000000;

    // check neighborhood type
    if( neighborhood == NEIGHBORHOOD_MOORE ) {
        neighbors.resize( 9 );
        result.resize( 1 );
        auto out_it = out;
        auto dl_it = in;
        auto dm_it = in;
        auto dr_it = in;
        // scan through image by column
        for( int x = 0; x < dim.x; x++ ) {
            // set intial neighborhood
            if( x == 0 ) {
                UL = *(in + ( dim.y - 1 ) * dim.x + dim.x - 1);
                ML = *(in + dim.x - 1);
                dl_it = in + (2 * dim.x - 1); 
            }
            else {
                UL = *(in + ( dim.y - 1 ) * dim.x + x - 1);
                ML = *(in + x - 1);
                dl_it = in + (dim.x + x - 1);
            }
            DL = *dl_it;
            UM = *(in + ( dim.y - 1 ) * dim.x + x);
            MM = *(in + x); 
            dm_it = in + (dim.x + x); DM = *dm_it;
            if( x == dim.x - 1 ) {
                UR = *(in + ( dim.y - 1 ) * dim.x);
                MR = *in; 
                dr_it = in + dim.x; 
            }
            else {
                UR = *(in + ( dim.y - 1 ) * dim.x + x + 1);
                MR = *(in + x + 1);
                dr_it = in + (dim.x + x + 1);
            }
            DR = *dr_it;
            out_it = out + x;
            for( int y = 0; y < dim.y - 1; y++ ) {
                rule( neighbors, result );  // apply rule
                *out_it = result[0];        // set output
                out_it += dim.x;
                // update neighborhood
                UL = ML; UM = MM; UR = MR;
                ML = DL; MM = DM; MR = DR;
                dl_it += dim.x; dm_it += dim.x; dr_it += dim.x;
                DL = *dl_it; DM = *dm_it; DR = *dr_it;
            }
            // set last row
            if( x == 0 ) DL = *(in + dim.x - 1); else DL = *(in + x - 1);
            DM = *(in + x);
            if( x == dim.x - 1 ) DR = *in; else DR = *(in + x + 1);
            rule( neighbors, result );  // apply rule
            *out_it = result[0];
        }

    } else if( neighborhood == NEIGHBORHOOD_VON_NEUMANN ) {
    } else if( neighborhood == NEIGHBORHOOD_MARGOLIS ) {
    } 
    buf.swap();
    return true;
}

/*
 *  Conway's Game of Life
 *  http://en.wikipedia.org/wiki/Conway's_Game_of_Life
 *  http://www.bitstorm.org/gameoflife/
 *  http://www.bitstorm.org/gameoflife/lexicon/
 */

template<class T> void life<T>::operator()(const std::vector<T>& neighbors, std::vector<T>& result) {    
    int count = 0;
    for( int i=0; i<4; i++ ) {
        if( neighbors[ i ] == on ) count++;
    }    
    for( int i=5; i<9; i++ ) {
        if( neighbors[ i ] == on ) count++;
    }
    if( MM == on ) {
        if( count == 2 || count == 3 ) result[0] = on;
        else result[0] = off;
    } else {
        if( count == 3 ) result[0] = on;
        else result[0] = off;
    }
}

//template class CA< frgb >;       // fimage
template class CA< ucolor >;     // uimage
//template class CA< vec2f >;      // vector_field

template class life< frgb >;       // fimage
template class life< ucolor >;     // uimage
template class life< vec2f >;      // vector_field


