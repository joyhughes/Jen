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

// Von Neumann neighborhood shortcuts
#define VNU neighbors[0]
#define VNL neighbors[1]
#define VNM neighbors[2]
#define VNR neighbors[3]
#define VND neighbors[4]

// Margolis neighborhood shortcuts
#define MUL neighbors[0]
#define MUR neighbors[1]
#define MLL neighbors[2]
#define MLR neighbors[3]

// future - implement multiresolution rule on mip-map

// Uses toroidal boundary conditions
template< class T > bool CA< T >::operator() ( buffer_pair<T> &buf, const float &t ) {
    if(!buf.has_image()) return false;
    auto in =  buf.get_image().begin();
    auto out = buf.get_buffer().begin();
    vec2i dim = buf.get_image().get_dim();

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
    } else if( neighborhood == NEIGHBORHOOD_MARGOLIS ) { // Works best if image dimensions are multiples of 2
        neighbors.resize( 4 );
        result.resize( 4 );
        auto out_ul = out; auto out_ur = out; auto out_ll = out; auto out_lr = out;
        auto in_ul = in; auto in_ur = in; auto in_ll = in; auto in_lr = in;
        // initialize iterators
        int startx, starty;
        if( frame % 2 ) { startx = 0; starty = 0; } // even frame - fits into upper left corner of image
        else            { startx = 1; starty = -1; } // odd frame - offset by 1 (initally straddles four corners of image)
        // scan through image by row
        for( int y = starty; y < dim.y - 1; y+=2 ) {
            if( frame % 2 ) { // even left edge
                out_ul = out + y * dim.x;       out_ur = out + y * dim.x + 1; 
                out_ll = out + (y + 1) * dim.x; out_lr = out + (y + 1) * dim.x + 1;

                in_ul = in + y * dim.x;         in_ur = in + y * dim.x + 1; 
                in_ll = in + (y + 1) * dim.x;   in_lr = in + (y + 1) * dim.x + 1;
            }
            else { // odd left edge
                if( y == -1 ) { // top row
                    out_ul = out + dim.y * dim.x - 1; out_ur = out + ( dim.y - 1 ) * dim.x; 
                    out_ll = out + dim.x - 1;         out_lr = out;

                    in_ul  = in  + dim.y * dim.x - 1; in_ur  = in  + ( dim.y - 1 ) * dim.x; 
                    in_ll  = in  + dim.x - 1;         in_lr  = in;

                    MUL = *in_ul; MUR = *in_ur; MLL = *in_ll; MLR = *in_lr;
                    rule( neighbors, result );  // apply rule
                    *out_ul = result[0]; *out_ur = result[1]; *out_ll = result[2]; *out_lr = result[3];

                    out_ul = out + ( dim.y - 1 ) * dim.x + 1; out_ur = out + ( dim.y - 1 ) * dim.x + 2; 
                    out_ll = out + 1;                         out_lr = out + 2;

                    in_ul  = in +  ( dim.y - 1 ) * dim.x + 1; in_ur  = in  + ( dim.y - 1 ) * dim.x + 2;
                    in_ll  = in  + 1;                         in_lr  = in  + 2;
                }
                else {
                    out_ul = out + (y + 1) * dim.x - 1; out_ur = out + y * dim.x; 
                    out_ll = out + (y + 2) * dim.x - 1; out_lr = out + (y + 1) * dim.x;

                    in_ul = in + (y + 1) * dim.x - 1;   in_ur = in + y * dim.x;
                    in_ll = in + (y + 2) * dim.x - 1;   in_lr = in + (y + 1) * dim.x;

                    MUL = *in_ul; MUR = *in_ur; MLL = *in_ll; MLR = *in_lr;
                    rule( neighbors, result );  // apply rule
                    *out_ul = result[0]; *out_ur = result[1]; *out_ll = result[2]; *out_lr = result[3];

                    out_ul = out + y * dim.x + 1;       out_ur = out + y * dim.x + 2; 
                    out_ll = out + (y + 1) * dim.x + 1; out_lr = out + (y + 1) * dim.x + 2;

                    in_ul = in + y * dim.x + 1;         in_ur = in + y * dim.x + 2;
                    in_ll = in + (y + 1) * dim.x + 1;   in_lr = in + (y + 1) * dim.x + 2;
                }
            }
            for( int x= startx; x < dim.x; x += 2 ) {
                // set neighborhood
                MUL = *in_ul; MUR = *in_ur; MLL = *in_ll; MLR = *in_lr;
                rule( neighbors, result );  // apply rule
                *out_ul = result[0]; *out_ur = result[1]; *out_ll = result[2]; *out_lr = result[3];
                // update neighborhood
                in_ul  += 2; in_ur  += 2; in_ll  += 2; in_lr  += 2;
                out_ul += 2; out_ur += 2; out_ll += 2; out_lr += 2;
            }
        }
    } 
    frame++;
    buf.swap();
    return true;
}

/*
 *  Conway's Game of Life
 *  http://en.wikipedia.org/wiki/Conway's_Game_of_Life
 *  http://www.bitstorm.org/gameoflife/
 *  http://www.bitstorm.org/gameoflife/lexicon/
 */

template< class T > void life< T >::operator () (const std::vector<T>& neighbors, std::vector<T>& result) {    
    int count = 0;
    for( int i = 0; i < 4; i++ ) { count += (neighbors[ i ] == on); }    
    for( int i = 5; i < 9; i++ ) { count += (neighbors[ i ] == on); } 
    if( MM == on ) {
        if( count == 2 || count == 3 ) result[0] = on;
        else result[0] = off;
    } else {
        if( count == 3 ) result[0] = on;
        else result[0] = off;
    }
}

template< class T > void diffuse< T >::operator () (const std::vector<T> &neighbors, std::vector<T> &result) {
    int r = rand_4( gen );
    
    if( alpha_block ) {
        bool blocked = false;
        for( int i = 0; i < 4; i++ ) blocked |= ( neighbors[ i ] &  0xff000000 ) != 0 ; 
        if( blocked ) {
            result.assign( neighbors.begin(), neighbors.end() );
            return;
        }
    }
    result[0] = neighbors[ r ];
    result[1] = neighbors[ (r + 1) % 4 ];
    result[2] = neighbors[ (r + 2) % 4 ];
    result[3] = neighbors[ (r + 3) % 4 ];
}

template class CA< frgb >;       // fimage
template class CA< ucolor >;     // uimage
template class CA< vec2f >;      // vector_field

template class life< frgb >;       // fimage
template class life< ucolor >;     // uimage
template class life< vec2f >;      // vector_field

//template class diffuse< frgb >;       // fimage
template class diffuse< ucolor >;     // uimage
//template class diffuse< vec2f >;      // vector_field

