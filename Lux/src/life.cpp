#include "life.hpp"
#include "frgb.hpp"
#include "ucolor.hpp"
#include "vect2.hpp"
#include "scene.hpp"
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
// Margolus neighborhood shortcuts (clockwise from upper left)

// Margolus neighborhood shortcuts (clockwise from upper left)
#define MUL neighbors[0]
#define MUR neighbors[1]
#define MLR neighbors[2]
#define MLL neighbors[3]

// Margolus neighborhood result shortcuts (clockwise from upper left)
#define RUL result[0]
#define RUR result[1]
#define RLR result[2]
#define RLL result[3]

// Margolus neighborhood target shortcuts (clockwise from upper left)
#define TUL targ[0]
#define TUR targ[1]
#define TLR targ[2]
#define TLL targ[3]

// Margolus Neighborhood pixel swaps
#define SWAP_LEFT  { RUL = MLL; RLL = MUL; }
#define SAME_LEFT  { RUL = MUL; RLL = MLL; }
#define SWAP_RIGHT { RUR = MLR; RLR = MUR; }
#define SAME_RIGHT { RUR = MUR; RLR = MLR; }
#define SWAP_UPPER { RUL = MUR; RUR = MUL; }
#define SAME_UPPER { RUL = MUL; RUR = MUR; }
#define SWAP_LOWER { RLL = MLR; RLR = MLL; }
#define SAME_LOWER { RLL = MLL; RLR = MLR; }
#define SWAP_DOWN_DIAG { RUL = MLR; RLR = MUL; RLL = MLL; RUR = MUR; }
#define SWAP_UP_DIAG   { RUL = MUL; RLR = MLR; RLL = MUR; RUR = MLL; }
#define SAME_ALL { RUL = MUL; RUR = MUR; RLL = MLL; RLR = MLR; }

// CA starting coordinates shortcuts
// future - add for clarity (if you dare!)
//#define IN(  x, y ) in  + ( ( y ) * dim.x + ( x ) )
//#define OUT( x, y ) out + ( ( y ) * dim.x + ( x ) )

/*
 template<class T> void CA<T>::set_rule( any_rule rule ) { 
    this->rule = rule; 
    this->hood = rule.hood;
}
*/

// evaluates conditions then executes rule
template< class T > void CA< T >::run_rule() {
    bool block = false;
    if( *p < 1.0f ) if( rand1( gen ) > *p ) block = true;
    if( *edge_block ) if( x <= 0 || x >= dim.x - 1 || y <= 0 || y >= dim.y - 1 ) block = true;
    //if( alpha_block ) 
    //if(image_block)
    if( *bright_block ) {
        if( hood == HOOD_MOORE) {
            unsigned int bright = ( ( MM >> 16 ) & 0xff ) + ( ( MM >> 8 ) & 0xff ) + ( MM & 0xff );
            if( bright < (*bright_range).min || bright > (*bright_range).max ) block = true;
        }
        else {  // Margolus family assumed
            for( int i = 0; i < 4; i++ ) {
                unsigned int bright = ( ( neighbors[ i ] >> 16 ) & 0xff ) + ( ( neighbors[ i ] >> 8 ) & 0xff ) + ( neighbors[ i ] & 0xff );
                if( bright < (*bright_range).min || bright > (*bright_range).max ) block = true;
            }
        }
    }
    if( block ) {
        if( hood == HOOD_MOORE) result[ 0 ] = MM;
        else result = neighbors; // Margolus family assumed
    }
    else {
        rule( *this );
    }
} 

// future - implement multiresolution rule on mip-map
// Uses toroidal boundary conditions
template< class T > void CA< T >::operator() ( any_buffer_pair_ptr& buf, element_context& context ) {

    if( ca_frame == 0 ) {
        ca_frame++;
        return;
    } 
    p( context ); 
    bright_block( context ); bright_range( context ); 
    edge_block( context ); alpha_block( context );
    std::cout << "CA: bright_block " << *bright_block << " bright_min " << (*bright_range).min << " bright_max " << (*bright_range).max << std::endl;
    hood = rule.init( context );
    if ( std::holds_alternative< std::shared_ptr< buffer_pair< T > > >( buf ) ) {
        auto buf_ptr = std::get< std::shared_ptr< buffer_pair< T > > >( buf ); 
        auto tar_ptr = buf_ptr;
        if( !buf_ptr->has_image() ) throw std::runtime_error( "CA: no image buffer" );
        auto img = buf_ptr->get_image();
        auto in =  img.begin();
        auto out = buf_ptr->get_buffer().begin();
        auto tar = in;
        if( targeted ) { 
            if( std::holds_alternative< std::shared_ptr< buffer_pair< T > > >( target ) ) {
                tar_ptr = std::get<     std::shared_ptr< buffer_pair< T > > >( target );
                // future: handle different target dimensions
                if( tar_ptr.get() ) {   // check for null pointer
                    if( tar_ptr->has_image() ) {
                        if( tar_ptr->get_image().get_dim() == img.get_dim() ) tar = tar_ptr->get_image().begin();
                        else {
                            std::cout << "CA: target buffer dimensions do not match" << std::endl;
                            throw std::runtime_error( "CA: target buffer dimensions do not match" );
                        }
                    }
                    else {
                        std::cout << "CA: target buffer has no image" << std::endl;
                        throw std::runtime_error( "CA: target buffer has no image" );
                    }
                }
                else {
                    std::cout << "CA: target buffer is null" << std::endl;
                    throw std::runtime_error( "CA: target buffer is null" );
                }
             }
        }
        dim = img.get_dim();

        // check neighborhood type
        if( hood == HOOD_MOORE ) {
            neighbors.resize( 9 );
            result.resize( 1 );
            if( targeted ) targ.resize( 1 );
            auto out_it = out;
            auto dl_it = in;
            auto dm_it = in;
            auto dr_it = in;
            auto tar_it = tar;
            // scan through image by column
            for( x = 0; x < dim.x; x++ ) {
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
                if( targeted ) tar_it = tar + x;
                for( y = 0; y < dim.y - 1; y++ ) {
                    run_rule();  // apply rule
                    if( targeted ) {    // if targeted, compare with target
                        if( manhattan( *tar_it, result[0] ) < manhattan( *tar_it, MM ) ) *out_it = result[0];
                        else *out_it = MM;
                    }
                    else *out_it = result[0];        // set output
                    out_it += dim.x;
                    if( targeted ) tar_it += dim.x;
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
                run_rule();  // apply rule
                if( targeted ) {    // if targeted, compare with target
                    if( manhattan( *tar_it, result[0] ) < manhattan( *tar_it, MM ) ) *out_it = result[0];
                    else *out_it = MM;
                }
                else *out_it = result[0];
            }
        } 
        // Margolus neighborhood family
        // Works best if image dimensions are multiples of 2
        else if((int)hood >= (int)HOOD_MARGOLUS ){ 
            neighbors.resize( 4 );
            result.resize( 4 );
            if( targeted ) targ.resize( 4 );
            auto out_ul = out; auto out_ur = out; auto out_ll = out; auto out_lr = out;
            auto in_ul  = in;  auto in_ur  = in;  auto in_ll  = in;  auto in_lr  = in;
            auto tar_ul = tar; auto tar_ur = tar; auto tar_ll = tar; auto tar_lr = tar;
            // initialize iterators
            int startx, starty;
            if( hood == HOOD_MARGOLUS ) { 
                if( ca_frame % 2 ) { startx = 0; starty = 0; } // even ca_frame - fits into upper left corner of image
                else               { startx = 1; starty = -1; } // odd ca_frame - offset by 1 (initally straddles four corners of image)
            }
            // 4-cycle Margolus offset neighborhood and variants
            else if( hood == HOOD_HOUR ) { 
                if( ca_frame % 2 )         startx = 0; else startx =  1;  // hourglass
                if( ( ca_frame / 2 ) % 2 ) starty = 0; else starty = -1;
            }
            else if( hood == HOOD_HOUR_REV ) { 
                if( ca_frame % 2 )         startx = 1; else startx =  0;  // reverse hourglass
                if( ( ca_frame / 2 ) % 2 ) starty = 0; else starty = -1;
            }
            else if( hood == HOOD_BOW ) { 
                if( ( ca_frame / 2 ) % 2 ) startx = 0; else startx = 1;  // bowtie
                if( ca_frame % 2 )         starty = 0; else starty = -1; 
            }
            else if( hood == HOOD_BOW_REV ) {  
                if( ( ca_frame / 2 ) % 2 ) startx =  0; else startx = 1;  // reverse bowtie
                if( ca_frame % 2 )         starty = -1; else starty = 0; 
            }
            else if( hood == HOOD_SQUARE ) { 
                if( ( ( 1 + ca_frame ) / 2 ) % 2 ) startx = 0; else startx = 1;  // square
                if( ( ca_frame / 2 )         % 2 ) starty = 0; else starty = -1;
            } 
            else if( hood == HOOD_SQUARE_REV ) { 
                if( ( ( 5 - ( ca_frame % 4 ) ) / 2 ) % 2 ) startx = 0; else startx = 1;  // square
                if( ( ca_frame / 2 )         % 2 ) starty = 0; else starty = -1;
            } 
            else if( hood == HOOD_RANDOM ) {
                if( fair_coin( gen ) ) startx = 0; else startx = 1;  // random
                if( fair_coin( gen ) ) starty = 0; else starty = -1;
            }
            // scan through image by row
            for( y = starty; y < dim.y - 1; y+=2 ) {
                if( !startx ) { // even left edge
                    if( y == -1 ) { // top row
                        out_ul = out + ( dim.y - 1 ) * dim.x; out_ur = out_ul + 1; 
                        out_ll = out;                         out_lr = out    + 1;

                        in_ul  = in  + ( dim.y - 1 ) * dim.x; in_ur  = in_ul  + 1; 
                        in_ll  = in;                          in_lr  = in     + 1;

                        if( targeted ) {
                            tar_ul = tar + ( dim.y - 1 ) * dim.x; tar_ur = tar_ul + 1; 
                            tar_ll = tar;                         tar_lr = tar    + 1;
                        }
                    }
                    else {
                        out_ul = out + y * dim.x;       out_ur = out_ul + 1; 
                        out_ll = out + (y + 1) * dim.x; out_lr = out_ll + 1;

                        in_ul = in + y * dim.x;         in_ur = in_ul + 1; 
                        in_ll = in + (y + 1) * dim.x;   in_lr = in_ll + 1;

                        if( targeted ) {
                            tar_ul = tar + y * dim.x;       tar_ur = tar_ul + 1; 
                            tar_ll = tar + (y + 1) * dim.x; tar_lr = tar_ll + 1;
                        }
                    }
                }
                else { // odd left edge
                    if( y == -1 ) { // top row, straddles corners
                        out_ul = out + dim.y * dim.x - 1; out_ur = out + ( dim.y - 1 ) * dim.x; 
                        out_ll = out + dim.x - 1;         out_lr = out;

                        in_ul  = in  + dim.y * dim.x - 1; in_ur  = in  + ( dim.y - 1 ) * dim.x; 
                        in_ll  = in  + dim.x - 1;         in_lr  = in;

                        if( targeted ) {
                            tar_ul = tar + dim.y * dim.x - 1; tar_ur = tar + ( dim.y - 1 ) * dim.x; 
                            tar_ll = tar + dim.x - 1;         tar_lr = tar;

                            TUL = *tar_ul; TUR = *tar_ur; TLL = *tar_ll; TLR = *tar_lr;
                        }

                        MUL = *in_ul; MUR = *in_ur; MLL = *in_ll; MLR = *in_lr;

                        run_rule();  // apply rule
                        if( targeted ) {
                            if( manhattan( RUL, TUL ) + manhattan( RUR, TUR ) + manhattan( RLR, TLR ) + manhattan( RLL, TLL ) <
                                manhattan( MUL, TUL ) + manhattan( MUR, TUR ) + manhattan( MLR, TLR ) + manhattan( MLL, TLL ) )
                                 { *out_ul = RUL; *out_ur = RUR; *out_ll = RLL; *out_lr = RLR; }
                            else { *out_ul = MUL; *out_ur = MUR; *out_ll = MLL; *out_lr = MLR; }
                        }
                        else *out_ul = RUL; *out_ur = RUR; *out_ll = RLL; *out_lr = RLR;

                        out_ul = out + ( dim.y - 1 ) * dim.x + 1; out_ur = out + ( dim.y - 1 ) * dim.x + 2; 
                        out_ll = out + 1;                         out_lr = out + 2;

                        in_ul  = in +  ( dim.y - 1 ) * dim.x + 1; in_ur  = in  + ( dim.y - 1 ) * dim.x + 2;
                        in_ll  = in  + 1;                         in_lr  = in  + 2;

                        if( targeted ) {
                            tar_ul = tar +  ( dim.y - 1 ) * dim.x + 1; tar_ur = tar + ( dim.y - 1 ) * dim.x + 2;
                            tar_ll = tar + 1;                         tar_lr = tar + 2;

                            TUL = *tar_ul; TUR = *tar_ur; TLL = *tar_ll; TLR = *tar_lr;
                        }
                    }
                    else {
                        out_ul = out + (y + 1) * dim.x - 1; out_ur = out + y * dim.x; 
                        out_ll = out + (y + 2) * dim.x - 1; out_lr = out + (y + 1) * dim.x;

                        in_ul = in + (y + 1) * dim.x - 1;   in_ur = in + y * dim.x;
                        in_ll = in + (y + 2) * dim.x - 1;   in_lr = in + (y + 1) * dim.x;

                        if( targeted ) {
                            tar_ul = tar + (y + 1) * dim.x - 1; tar_ur = tar + y * dim.x; 
                            tar_ll = tar + (y + 2) * dim.x - 1; tar_lr = tar + (y + 1) * dim.x;

                            TUL = *tar_ul; TUR = *tar_ur; TLL = *tar_ll; TLR = *tar_lr;
                        }

                        MUL = *in_ul; MUR = *in_ur; MLL = *in_ll; MLR = *in_lr;
                        run_rule();  // apply rule
                        if( targeted ) {
                            if( manhattan( RUL, TUL ) + manhattan( RUR, TUR ) + manhattan( RLR, TLR ) + manhattan( RLL, TLL ) <
                                manhattan( MUL, TUL ) + manhattan( MUR, TUR ) + manhattan( MLR, TLR ) + manhattan( MLL, TLL ) )
                                 { *out_ul = RUL; *out_ur = RUR; *out_ll = RLL; *out_lr = RLR; }
                            else { *out_ul = MUL; *out_ur = MUR; *out_ll = MLL; *out_lr = MLR; }
                        }
                        else *out_ul = RUL; *out_ur = RUR; *out_ll = RLL; *out_lr = RLR;
                        out_ul = out + y * dim.x + 1;       out_ur = out + y * dim.x + 2; 
                        out_ll = out + (y + 1) * dim.x + 1; out_lr = out + (y + 1) * dim.x + 2;

                        in_ul = in + y * dim.x + 1;         in_ur = in + y * dim.x + 2;
                        in_ll = in + (y + 1) * dim.x + 1;   in_lr = in + (y + 1) * dim.x + 2;

                        if( targeted ) {
                            tar_ul = tar + y * dim.x + 1;         tar_ur = tar + y * dim.x + 2;
                            tar_ll = tar + (y + 1) * dim.x + 1;   tar_lr = tar + (y + 1) * dim.x + 2;

                            TUL = *tar_ul; TUR = *tar_ur; TLL = *tar_ll; TLR = *tar_lr;
                        }
                    }
                }
                for( x= startx; x < dim.x; x += 2 ) {
                    // set neighborhood
                    MUL = *in_ul; MUR = *in_ur; MLL = *in_ll; MLR = *in_lr;
                    if( targeted ) { TUL = *tar_ul; TUR = *tar_ur; TLL = *tar_ll; TLR = *tar_lr; }
                    run_rule();  // apply rule
                    *out_ul = RUL; *out_ur = RUR; *out_ll = RLL; *out_lr = RLR;
                    // update neighborhood
                    in_ul  += 2; in_ur  += 2; in_ll  += 2; in_lr  += 2;
                    out_ul += 2; out_ur += 2; out_ll += 2; out_lr += 2;
                    if( targeted ) { tar_ul += 2; tar_ur +=2 ; tar_ll +=2 ; tar_lr += 2; }
                    if( x == dim.x - 3 ) { // right edge
                        in_ur  -= dim.x; in_lr  -= dim.x;
                        out_ur -= dim.x; out_lr -= dim.x;
                        if( targeted ) { tar_ur -= dim.x; tar_lr -= dim.x; }
                    }
                }
            }
        } 
        ca_frame++;
        buf_ptr->swap();
    }
} 

template< class T > CA_hood rule_identity< T >::operator () ( element_context &context )
{ return HOOD_MARGOLUS; }

template< class T > void rule_identity< T >::operator () ( CA< T >& ca ) { 
    auto& neighbors = ca.neighbors;
    auto& result = ca.result;

    result.assign( neighbors.begin(), neighbors.end() );
}

/*
 *  Conway's Game of Life
 *  http://en.wikipedia.org/wiki/Conway's_Game_of_Life
 *  http://www.bitstorm.org/gameoflife/
 *  http://www.bitstorm.org/gameoflife/lexicon/
 */

template<class T> inline rule_life<T>::rule_life() { 
    T c;
    black( c );
    on = c;
    white( c );
    off = c;
}

template< class T > CA_hood rule_life< T >::operator () ( element_context &context ) {
    on( context ); off( context );
    return HOOD_MOORE;
}

template< class T > void rule_life< T >::operator () ( CA< T >& ca ) { 
    auto& neighbors = ca.neighbors;
    auto& result = ca.result;

    int count = 0;
    for( int i = 0; i < 4; i++ ) { count += (neighbors[ i ] == *on); }    
    for( int i = 5; i < 9; i++ ) { count += (neighbors[ i ] == *on); } 
    if( MM == *on ) {
        if( count == 2 || count == 3 ) result[0] = *on;
        else result[0] = *off;
    } else {
        if( count == 3 ) result[0] = *on;
        else result[0] = *off;
    }
}

template< class T > CA_hood rule_random_copy< T >::operator () ( element_context &context ) 
{ 
    return HOOD_MOORE; 
}

template< class T > void rule_random_copy< T >::operator () ( CA< T >& ca ) {
    auto& neighbors = ca.neighbors;
    auto& result = ca.result;
    int r = rand_9( gen );
    result[0] = neighbors[ r ]; // copy any neighbor, including possibly itself
}

template< class T > CA_hood rule_random_mix< T >::operator () ( element_context &context ) 
{ 
    return HOOD_MOORE; 
}

template< class T > void rule_random_mix< T >::operator () ( CA< T >& ca ) {
    auto& neighbors = ca.neighbors;
    auto& result = ca.result;
    result[ 0 ] = 0;
    int r = rand_9( gen );
    result[0] |= neighbors[ r ] & 0x00ff0000; // copy any neighbor, including possibly itself
    r = rand_9( gen );
    result[0] |= neighbors[ r ] & 0x0000ff00; // copy any neighbor, including possibly itself
    r = rand_9( gen );
    result[0] |= neighbors[ r ] & 0x000000ff; // copy any neighbor, including possibly itself
}

template< class T > CA_hood rule_diffuse< T >::operator () ( element_context &context ) 
{ return HOOD_MARGOLUS; }

template< class T > void rule_diffuse< T >::operator () ( CA< T >& ca ) {
    auto& neighbors = ca.neighbors;
    auto& result = ca.result;
    int r = rand_4( gen );
    
    result[0] = neighbors[ r ];
    result[1] = neighbors[ (r + 1) % 4 ];
    result[2] = neighbors[ (r + 2) % 4 ];
    result[3] = neighbors[ (r + 3) % 4 ];
}

template< class T > CA_hood rule_gravitate< T >::operator () ( element_context &context )
{ return HOOD_MARGOLUS; } 

template< class T > void rule_gravitate< T >::operator () ( CA< T >& ca ) {
    int r;
    auto& neighbors = ca.neighbors;
    auto& result = ca.result;


    // Calculate approximate brighness of pixels (sum of color components)
    int wul = ((MUL & 0x00ff0000) >> 16) + ((MUL & 0x0000ff00) >> 8) + (MUL & 0x000000ff);
    int wur = ((MUR & 0x00ff0000) >> 16) + ((MUR & 0x0000ff00) >> 8) + (MUR & 0x000000ff);
    int wll = ((MLL & 0x00ff0000) >> 16) + ((MLL & 0x0000ff00) >> 8) + (MLL & 0x000000ff);
    int wlr = ((MLR & 0x00ff0000) >> 16) + ((MLR & 0x0000ff00) >> 8) + (MLR & 0x000000ff);

    // Sort pixels by brightness
    int wu = wul + wur;
    int wd = wll + wlr;
    int wl = wul + wll;
    int wr = wur + wlr;

    int udiff = wu - wd;
    int ldiff = wl - wr;

    if( abs( udiff ) > abs( ldiff ) ) {
        if( udiff > 0 ) r = 0;
        else r = 2;
    } else {
        if( ldiff > 0 ) r = 3;
        else r = 1;
    }
    r = *direction - r + 4;

    result[0] = neighbors[ r ];
    result[1] = neighbors[ (r + 1) % 4 ];
    result[2] = neighbors[ (r + 2) % 4 ];
    result[3] = neighbors[ (r + 3) % 4 ];
} 

template< class T > CA_hood rule_snow< T >::operator () ( element_context &context ) 
{   
    direction( context );
    return HOOD_MARGOLUS; 
}

// Bug preserved in amber. A version of gravitate with a bug that causes it to rotate in the opposite direction.
template< class T > void rule_snow< T >::operator () ( CA< T >& ca ) {
    int r;
    auto& neighbors = ca.neighbors;
    auto& result = ca.result;

    // Calculate approximate brighness of pixels (sum of color components)
    int wul = ((MUL & 0x00ff0000) >> 16) + ((MUL & 0x0000ff00) >> 8) + (MUL & 0x000000ff);
    int wur = ((MUR & 0x00ff0000) >> 16) + ((MUR & 0x0000ff00) >> 8) + (MUR & 0x000000ff);
    int wll = ((MLL & 0x00ff0000) >> 16) + ((MLL & 0x0000ff00) >> 8) + (MLL & 0x000000ff);
    int wlr = ((MLR & 0x00ff0000) >> 16) + ((MLR & 0x0000ff00) >> 8) + (MLR & 0x000000ff);

    // Sort pixels by brightness
    int wu = wul + wur;
    int wd = wll + wlr;
    int wl = wul + wll;
    int wr = wur + wlr;

    int udiff = wu - wd;
    int ldiff = wl - wr;

    if( abs( udiff ) > abs( ldiff ) ) {
        if( udiff > 0 ) r = 0;
        else r = 2;
    } else {
        if( ldiff > 0 ) r = 1;
        else r = 3;
    }
    r = *direction - r + 4;

    r = *direction - r + 4;

    result[0] = neighbors[ r % 4 ];
    result[1] = neighbors[ (r + 1) % 4 ];
    result[2] = neighbors[ (r + 2) % 4 ];
    result[3] = neighbors[ (r + 3) % 4 ];
}

template< class T > CA_hood rule_pixel_sort< T >::operator () ( element_context &context ) 
{ 
    direction( context );
    max_diff( context );
    if( diagonal( *direction ) ) return HOOD_HOUR; 
    else                        return HOOD_MARGOLUS;
}

template< class T > void rule_pixel_sort< T >::operator () ( CA< T >& ca ) {
    auto& neighbors = ca.neighbors;
    auto& result = ca.result;

    // Calculate approximate brighness of pixels (sum of color components)
    int wul = ((MUL & 0x00ff0000) >> 16) + ((MUL & 0x0000ff00) >> 8) + (MUL & 0x000000ff);
    int wur = ((MUR & 0x00ff0000) >> 16) + ((MUR & 0x0000ff00) >> 8) + (MUR & 0x000000ff);
    int wll = ((MLL & 0x00ff0000) >> 16) + ((MLL & 0x0000ff00) >> 8) + (MLL & 0x000000ff);
    int wlr = ((MLR & 0x00ff0000) >> 16) + ((MLR & 0x0000ff00) >> 8) + (MLR & 0x000000ff);

    if( diagonal( *direction ) ) {
        if( *direction == direction8::D8_DOWNRIGHT || *direction == direction8::D8_UPLEFT ){
            if( ( ( wul > wlr ) == ( *direction == direction8::D8_DOWNRIGHT ) ) && ( manhattan( MUL, MLR ) < *max_diff ) ) 
                SWAP_DOWN_DIAG else SAME_ALL
        } else {
            if( ( ( wur > wll ) == ( *direction == direction8::D8_UPRIGHT  ) ) && ( manhattan( MUR, MLL ) < *max_diff ) )
                SWAP_UP_DIAG   else SAME_ALL
        }
    } else {
        if( vertical( *direction ) ) {
            if( ( ( wul > wll ) == ( *direction == direction8::D8_DOWN  ) ) && (  manhattan( MLL, MUL ) < *max_diff ) ) 
                SWAP_LEFT else SAME_LEFT
            if( ( ( wur > wlr ) == ( *direction == direction8::D8_DOWN  ) ) && (  manhattan( MLR, MUR ) < *max_diff ) )
                SWAP_RIGHT else SAME_RIGHT
        } else {
            if( ( ( wul > wur ) == ( *direction == direction8::D8_RIGHT ) ) && ( manhattan( MUL, MUR ) < *max_diff ) ) 
                SWAP_UPPER else SAME_UPPER
            if( ( ( wll > wlr ) == ( *direction == direction8::D8_RIGHT ) ) && ( manhattan( MLR, MLL ) < *max_diff ) ) 
                SWAP_LOWER else SAME_LOWER
        }
    } 
}

template< class T > CA_hood rule_funky_sort< T >::operator () ( element_context &context ) 
{ 
    direction( context );
    max_diff( context );
    return hood;
    //return HOOD_HOUR; 
    //if( diagonal( direction ) ) return HOOD_HOUR; 
    //else                        return HOOD_MARGOLUS;
} 

template< class T > void rule_funky_sort< T >::operator () ( CA< T >& ca ) { 
    auto& neighbors = ca.neighbors;
    auto& result = ca.result;

    // Rotate neighbors opposite direction
    if( *direction == D8_RIGHT || *direction == D8_DOWNRIGHT ) {
        T tmp = MUL;
        MUL = MUR;
        MUR = MLR;
        MLR = MLL;
        MLL = tmp;
    }
    else if( *direction == D8_DOWN || *direction == D8_DOWNLEFT ) {
        std::swap( MUL, MLR );
        std::swap( MUR, MLL );
    }
    else if( *direction == D8_LEFT || *direction == D8_UPLEFT ) {
        T tmp = MUL;
        MUL = MLL;
        MLL = MLR;
        MLR = MUR;
        MUR = tmp;
    }

    // Calculate approximate brighness of pixels (sum of color components)
    int wul = ((MUL & 0x00ff0000) >> 16) + ((MUL & 0x0000ff00) >> 8) + (MUL & 0x000000ff);
    int wur = ((MUR & 0x00ff0000) >> 16) + ((MUR & 0x0000ff00) >> 8) + (MUR & 0x000000ff);
    int wll = ((MLL & 0x00ff0000) >> 16) + ((MLL & 0x0000ff00) >> 8) + (MLL & 0x000000ff);
    int wlr = ((MLR & 0x00ff0000) >> 16) + ((MLR & 0x0000ff00) >> 8) + (MLR & 0x000000ff);

    unsigned int funk =   
        ( (unsigned int)( manhattan( MUR, MLR ) < *max_diff ) ) | 
        ( (unsigned int)( manhattan( MLR, MLL ) < *max_diff ) << 1 ) | 
        ( (unsigned int)( manhattan( MLL, MUL ) < *max_diff ) << 2 ) | 
        ( (unsigned int)( manhattan( MUL, MUR ) < *max_diff ) << 3 ) | 
        ( (unsigned int)( manhattan( MUL, MLR ) < *max_diff ) << 4 ) | 
        ( (unsigned int)( manhattan( MUR, MLL ) < *max_diff ) << 5 );

    if( !diagonal( *direction ) ) {
        if( ( ( dafunk_l >> funk ) & 1 ) && ( wll > wul ) ) SWAP_LEFT  else SAME_LEFT
        if( ( ( dafunk_r >> funk ) & 1 ) && ( wlr > wur ) ) SWAP_RIGHT else SAME_RIGHT
    }
    else { 
        if( ( ( dafunk_d >> funk ) & 1 ) && ( wll > wur ) ) SWAP_UP_DIAG else SAME_ALL
    }

    // Rotate result same direction
    if( *direction == D8_RIGHT || *direction == D8_DOWNRIGHT ) {
        T tmp = RUL;
        RUL = RLL;
        RLL = RLR;
        RLR = RUR;
        RUR = tmp;
    }
    else if( *direction == D8_DOWN || *direction == D8_DOWNLEFT ) {
        std::swap( RUL, RLR );
        std::swap( RUR, RLL );
    }
    else if( *direction == D8_LEFT || *direction == D8_UPLEFT ) {
        T tmp = RUL;
        RUL = RUR;
        RUR = RLR;
        RLR = RLL;
        RLL = tmp;
    }
} 

/* Older, more complex implementation
#define FUNK( B5, B4, B3, B2, B1, B0 ) \
    funk =  ( (unsigned int)( manhattan( MUR, MLR ) < *max_diff ) << (B0) ) | \
            ( (unsigned int)( manhattan( MLR, MLL ) < *max_diff ) << (B1) ) | \
            ( (unsigned int)( manhattan( MLL, MUL ) < *max_diff ) << (B2) ) | \
            ( (unsigned int)( manhattan( MUL, MUR ) < *max_diff ) << (B3) ) | \
            ( (unsigned int)( manhattan( MUL, MLR ) < *max_diff ) << (B4) ) | \
            ( (unsigned int)( manhattan( MUR, MLL ) < *max_diff ) << (B5) );

// Pixel sorting with dafunk
template< class T > void rule_funky_sort< T >::operator () ( CA< T >& ca ) {
    auto& neighbors = ca.neighbors;
    auto& result = ca.result;

    // Calculate approximate brighness of pixels (sum of color components)
    int wul = ((MUL & 0x00ff0000) >> 16) + ((MUL & 0x0000ff00) >> 8) + (MUL & 0x000000ff);
    int wur = ((MUR & 0x00ff0000) >> 16) + ((MUR & 0x0000ff00) >> 8) + (MUR & 0x000000ff);
    int wll = ((MLL & 0x00ff0000) >> 16) + ((MLL & 0x0000ff00) >> 8) + (MLL & 0x000000ff);
    int wlr = ((MLR & 0x00ff0000) >> 16) + ((MLR & 0x0000ff00) >> 8) + (MLR & 0x000000ff);

    // Sort pixels by brightness if funky condition satisfied
    unsigned int funk;
    if( !((unsigned int)direction / 4) ) { // First four directions UP, UP_RIGHT, RIGHT, DOWN_RIGHT
        if( direction == D8_UP || direction == D8_UPRIGHT ) { // Directions UP, UP_RIGHT
            FUNK( 5, 4, 3, 2, 1, 0 ) 
            //std::cout << "funk " << funk << " dafunk_d " << dafunk_d << " direction " << direction << std::endl;
            if( direction == D8_UP ) {
                if( ( ( dafunk_l >> funk ) & 1 ) && ( wll > wul ) ) SWAP_LEFT  else SAME_LEFT
                if( ( ( dafunk_r >> funk ) & 1 ) && ( wlr > wur ) ) SWAP_RIGHT else SAME_RIGHT
            }
            else { // Direction UP_RIGHT
                if( ( ( dafunk_d >> funk ) & 1 ) && ( wll > wur ) ) SWAP_UP_DIAG else SAME_ALL
            }
        }
        else { // Directions RIGHT, DOWN_RIGHT
            FUNK( 4, 5, 2, 1, 0, 3 )
            //FUNK( 4, 5, 0, 3, 2, 1 )

            if( direction == D8_RIGHT ) {
                if( ( ( dafunk_l >> funk ) & 1 ) && ( wul > wur ) ) SWAP_UPPER else SAME_UPPER
                if( ( ( dafunk_r >> funk ) & 1 ) && ( wll > wlr ) ) SWAP_LOWER else SAME_LOWER
            }
            else { // Direction DOWN_RIGHT
                if( ( ( dafunk_d >> funk ) & 1 ) && ( wul > wlr ) ) SWAP_DOWN_DIAG else SAME_ALL
            }
        }
    }
    else { // Last four directions DOWN, DOWN_LEFT, LEFT, UP_LEFT
        if( direction == D8_DOWN || direction == D8_DOWNLEFT ) { // Directions DOWN, DOWN_LEFT
            FUNK( 5, 4, 1, 0, 3, 2 )
            if( direction == D8_DOWN ) {
                if( ( ( dafunk_r >> funk ) & 1 ) && ( wul > wll ) ) SWAP_LEFT  else SAME_LEFT
                if( ( ( dafunk_l >> funk ) & 1 ) && ( wur > wlr ) ) SWAP_RIGHT else SAME_RIGHT
            }
            else { // Direction DOWN_LEFT
                if( ( ( dafunk_d >> funk ) & 1 ) && ( wur > wll ) ) SWAP_UP_DIAG else SAME_ALL
            }
        }
        else { // Directions LEFT, UP_LEFT
            FUNK( 4, 5, 0, 3, 2, 1 )
            //FUNK( 4, 5, 2, 1, 0, 3 )

            if( direction == D8_LEFT ) {
                if( ( ( dafunk_r >> funk ) & 1 ) && ( wur > wul ) ) SWAP_UPPER else SAME_UPPER
                if( ( ( dafunk_l >> funk ) & 1 ) && ( wlr > wll ) ) SWAP_LOWER else SAME_LOWER
            }
            else { // Direction UP_LEFT
                if( ( ( dafunk_d >> funk ) & 1 ) && ( wlr > wul ) ) SWAP_DOWN_DIAG else SAME_ALL
            }
        }
    }
}
*/

/* webs
                if( ( ( wul > wll ) == ( direction == direction8::D8_DOWN  ) ) && (  manhattan( MLL, MUR ) < *max_diff ) ) 
                     { RUL = MLL; RLL = MUL; } // swap left colum
                else { RUL = MUL; RLL = MLL; }
                if( ( ( wur > wlr ) == ( direction == direction8::D8_DOWN  ) ) && (  manhattan( MLR, MUL ) < *max_diff ) )
                     { RUR = MLR; RLR = MUR; } // swap right column
                else { RUR = MUR; RLR = MLR; }*/

/*  Cheese grater, flow, trees
          if( ( ( wur > wll ) == ( direction == direction8::D8_DOWNLEFT ) ) && ( ( manhattan( MUR, MLL ) < *max_diff ) || ( manhattan( MLL, MLR ) < *max_diff )  == ( manhattan( MLL, MUL ) < *max_diff ) ) )
                         { RUL = MUL; RLR = MLR; RLL = MUR; RUR = MLL; } // swap diagonal
                    else { RUL = MUL; RLR = MLR; RLL = MLL; RUR = MUR; }
            }*/
/* column percolation
        if( vertical( direction ) ) {
            if( ( ( wul > wll ) == ( direction == direction8::D8_DOWN ) ) && ( ( manhattan( MLL, MUL ) < *max_diff ) || ( manhattan( MLR, MUR ) < *max_diff ) ) )
                 { RUL = MLL; RLL = MUL; } // swap left column
            else { RUL = MUL; RLL = MLL; }
            if( ( ( wur > wlr ) == ( direction == direction8::D8_DOWN ) ) && ( ( manhattan( MLL, MUL ) < *max_diff ) || ( manhattan( MLR, MUR ) < *max_diff ) ) ) 
                 { RUR = MLR; RLR = MUR; } // swap right column
            else { RUR = MUR; RLR = MLR; }
        } else { */
        
/* Closest thing to orthogonal borg
 if( vertical( direction ) ) {
                if( ( ( wul > wll ) == ( direction == direction8::D8_DOWN  ) ) && ( ( manhattan( MLL, MUL ) < *max_diff ) || ( ( manhattan( MLL, MUR ) < *max_diff ) && ( manhattan( MLR, MLL ) < *max_diff ) ) ) ) 
                     { RUL = MLL; RLL = MUL; } // swap left colum
                else { RUL = MUL; RLL = MLL; }
                if( ( ( wur > wlr ) == ( direction == direction8::D8_DOWN  ) ) && ( ( manhattan( MLR, MUR ) < *max_diff ) || ( ( manhattan( MLR, MUL ) < *max_diff ) && ( manhattan( MUL, MUR ) < *max_diff ) ) ) )
                     { RUR = MLR; RLR = MUR; } // swap right column
                else { RUR = MUR; RLR = MLR; }
            } else {
                */
// crosstalk
//             if( ( ( wul > wll ) == ( direction == direction8::D8_DOWN ) ) && ( ( manhattan( MLL, MUL ) < *max_diff ) || ( manhattan( MLR, MUR ) < *max_diff ) ) )

/* Gorgeous! (city in the stars)
        if( !horizontal_direction8( direction ) ) {
            if( ( ( wul > wll ) == ( direction == direction8::D8_DOWN ) ) && ( ( manhattan( MLL, MUL ) < *max_diff ) || ( manhattan( MUL, MLR ) < *max_diff ) || ( manhattan( MUR, MLL ) < *max_diff )) ) 
*/

/*

template< class T > void rule_funky_sort< T >::operator () ( CA< T >& ca ) {
    auto& neighbors = ca.neighbors;
    auto& result = ca.result;
    if( luminance( MUL ) < 10 || luminance( MUR ) < 10 || luminance( MLL ) < 10 || luminance( MLR ) < 10) {
        result.assign( neighbors.begin(), neighbors.end() );
        return;
    }
    // Calculate approximate brighness of pixels (sum of color components)
    int wul = ((MUL & 0x00ff0000) >> 16) + ((MUL & 0x0000ff00) >> 8) + (MUL & 0x000000ff);
    int wur = ((MUR & 0x00ff0000) >> 16) + ((MUR & 0x0000ff00) >> 8) + (MUR & 0x000000ff);
    int wll = ((MLL & 0x00ff0000) >> 16) + ((MLL & 0x0000ff00) >> 8) + (MLL & 0x000000ff);
    int wlr = ((MLR & 0x00ff0000) >> 16) + ((MLR & 0x0000ff00) >> 8) + (MLR & 0x000000ff);

    // Sort pixels by brightness

    if( diagonal( direction ) ) {
        if( direction == D8_DOWNRIGHT || direction == D8_UPLEFT ){
            if( ( ( wul > wlr ) == ( direction == direction8::D8_DOWNRIGHT ) ) && ( manhattan( MUL, MLR ) < *max_diff ) ) 
                 { RUL = MLR; RLR = MUL; RLL = MLL; RUR = MUR; } // swap diagonal
            else { RUL = MUL; RLR = MLR; RLL = MLL; RUR = MUR; }
        }
        else {
            if( ( ( wur > wll ) == ( direction == direction8::D8_DOWNLEFT ) ) && ( ( manhattan( MUR, MLL ) < *max_diff ) || ( ( manhattan( MLL, MLR ) < *max_diff ) ) && ( manhattan( MLL, MUL ) < *max_diff ) ) )
                 { RUL = MUL; RLR = MLR; RLL = MUR; RUR = MLL; } // swap diagonal
            else { RUL = MUL; RLR = MLR; RLL = MLL; RUR = MUR; }
        }
    }
    else {
        if( !horizontal( direction ) ) {
            if( ( ( wul > wll ) == ( direction == direction8::D8_DOWN ) ) && ( manhattan( MLL, MUL ) < *max_diff ) ) 
                 { RUL = MLL; RLL = MUL; } // swap left column
            else { RUL = MUL; RLL = MLL; }
            if( ( ( wur > wlr ) == ( direction == direction8::D8_DOWN ) ) && ( manhattan( MLR, MUR ) < *max_diff ) ) 
                 { RUR = MLR; RLR = MUR; } // swap right column
            else { RUR = MUR; RLR = MLR; }
        }
        else {
            if( ( ( wul > wur ) == ( direction == direction8::D8_RIGHT ) ) && ( manhattan( MUL, MUR ) < *max_diff ) ) 
                { RUL = MUR; RUR = MUL; } // swap upper row
            else { RUL = MUL; RUR = MUR; }
            if( ( ( wll > wlr ) == ( direction == direction8::D8_RIGHT ) ) && ( manhattan( MLR, MLL ) < *max_diff ) ) 
                { RLL = MLR; RLR = MLL; } // swap lower row
            else { RLL = MLL; RLR = MLR; }
        }
    }
}
*/

template class CA< ucolor >;     // uimage

template class rule_identity< ucolor >;       // uimage
template class rule_life< ucolor >;     // uimage
template class rule_random_copy< ucolor >;     // uimage
template class rule_random_mix< ucolor >;     // uimage
template class rule_diffuse< ucolor >;     // uimage
template class rule_gravitate< ucolor >;       // uimage
template class rule_snow< ucolor >;       // uimage
template class rule_pixel_sort< ucolor >;       // uimage  
template class rule_funky_sort< ucolor >;       // uimage 