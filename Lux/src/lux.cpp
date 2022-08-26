#include <iostream>
#include <optional>
#include "linalg.h"
#include "vect2.hpp"
#include "vector_field.hpp"
#include "frgb.hpp"
#include "fimage.hpp"
#include "ucolor.hpp"
#include "uimage.hpp"
#include "scene.hpp"
#include "next_element.hpp"
#include "warp.hpp"

#include <unistd.h>
#include <sstream>
#include <string>
#include <iomanip>

unsigned long long getTotalSystemMemory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

void test_frgb() {
    using std::cout;
    using namespace linalg;
    using namespace ostream_overloads;

    cout << "TESTING FRGB\n\n";
    frgb color( -1.5f, 0.5f, 2.5f );
    cout << "Initial color     " << color << "\n\n";
    color = clamp( color, -1.0f, 2.0f );
    cout << "Clamped color     " << color << "\n\n";
    constrain( color );
    cout << "Constrained color " << color << "\n\n";
    print_SRGB( color );

    frgb ucolor = setc( (unsigned char)0x00, (unsigned char)0x80, (unsigned char)0xff );
    cout << "\n\nColor from unsigned char " << ucolor << "\n\n";
    print_SRGB( ucolor );
    cout << "\n\n";

    ucolor += color;
    cout << "Color added with += " << ucolor << "\n\n";

    frgb acolor = ucolor + color;
    cout << "Color added with +  " << acolor << "\n\n";

    color[ 0 ] = 0.5;
    cout << "Color after color[ 0 ] = 0.5 " << color << "\n\n";

    frgb result( { 0.0, 0.5, 1.0 } );
    frgb mask( { -2.0, 1.0, 2.0 } );
    frgb base( { 1.0, 0.5, 0.0 } );
    apply_mask( result, base, mask );

    cout << "Color mask result " << result << "\n\n";
}

void test_vect2() {
    using std::cout;
    using namespace linalg;
    using namespace ostream_overloads;

    cout << "\nTESTING VECT2\n\n";

    vec2f a = { 1.0f, 2.0f };
    cout << "vector a " << a << "\n";

    a = normalize( a );
    cout << "normalized " << a << "\n";

    auto b = rot( 1.0f, a );
    cout << "rotated with rot() " << b << "\n";

    mat2f rm = rotation_matrix_2D< float >( 1.0f );

    auto c = mul( rm, a );
    cout << "rotated with matrix " << c << "\n";

    auto d = less( vec2f( { 1.0f, 0.0f } ), vec2f( { 0.0f, 1.0f } ) );
    cout << "less test " << d << "\n";

    bb2f bb( vec2f( -1.0f, -1.0f ), vec2f( 1.0f, 1.0f ) );
    vec2f e(  0.5f, 0.5f );
    vec2f f( -1.5f, 1.5f );
    cout << "{ 0.5, 0.5 }  in bounds        " << bb.in_bounds( e )     << "\n";
    cout << "{ -1.5, 1.5 } in bounds        " << bb.in_bounds( f )     << "\n";
    bb.pad( 1.0f );
    cout << "{ 0.5, 0.5 }  in padded bounds " << bb.in_bounds_pad( e ) << "\n";
    cout << "{ -1.5, 1.5 } in padded bounds " << bb.in_bounds_pad( f ) << "\n";

    cout << "\nbox_of_random() test\n";
    for( int i = 0; i < 10; i++ ) { cout << bb.box_of_random() << " "; }
    cout << "\n\n";

    bb2i bbi( { 0, 0 }, { 99, 99 } );
    cout << "Mapped vector 1 " << bb.bb_map( { 50, 50 }, bbi ) << "\n";
    bb2i bbi2( { 0, 0 }, { 79, 119 } );
    cout << "Mapped vector 2 " << bbi2.bb_map( { 50, 50 }, bbi ) << "\n";

    bb2f bb2( bbi2 );
    cout << "Converted bounding box " << bb2.b1 << " " << bb2.b2 << "\n";

    vec2f j( { 5.5f, 5.5f } );
    vec2f k( { 1.0f, 2.0f } );
    cout << "fmod test " << linalg::fmod( j, k ) << "\n";

    cout << "\n\n";

}

void test_fimage() {
    using std::cout;
    cout << "\nTESTING FIMAGE\n\n";

    fimage a;
    a.load( "../../Jen-C/hk_square.jpg" ); 
    a.write_jpg( " hk_try.jpg", 100 );

    fimage b( a );
    cout << "image size comparison " << b.compare_dims( a ) << "\n";

    b.grayscale();
    b.write_jpg( " hk_gray.jpg", 100 );

    b *= { 0.3f, 0.6f, 1.0f };
    b.write_jpg( " hk_tint.jpg", 100 );

    b *= 1.5f;
    b.write_jpg( " hk_bright.jpg", 100 );

    b -= a;
    b.write_jpg( " hk_diff.jpg", 100 );

}

void  test_ucolor() {
    using std::cout;

    cout << "\nTESTING UCOLOR\n\n";
    ucolor a = 0xff4080ff;
    cout << std::hex << (int)rc( a ) << " " << (int)gc( a ) << " " << (int)bc( a ) << "\n\n";
}

void test_uimage() {
    using std::cout;
    cout << "\nTESTING UIMAGE\n\n";

    uimage a;
    a.load( "../../Jen-C/hk_square.jpg" ); 
    a.write_jpg( " hk_utry.jpg", 100 );

    uimage b( a );
    cout << "image size comparison " << b.compare_dims( a ) << "\n";

    b.grayscale();
    b.write_jpg( " hk_ugray.jpg", 100 );
/*
    b *= { 0.3f, 0.6f, 1.0f };
    b.write_jpg( " hk_tint.jpg", 100 );

    b *= 1.5f;
    b.write_jpg( " hk_bright.jpg", 100 );

    b -= a;
    b.write_jpg( " hk_diff.jpg", 100 );
*/
}

void test_vector_field() {
    using std::cout;
    cout << "\nTESTING VECTOR_FIELD\n\n";

    fimage a;
    a.load( "../../Jen-C/hk_square.jpg" ); 
    fimage b( a );
    //std::unique_ptr< image< frgb > > aptr( &a );
    //std::unique_ptr< image< frgb > > bptr( &b );
    vector_field vf( b.get_dim() / 4 );
    vortex_field vortf( 20, true );
    vortf.min_intensity = 0.1f; vortf.max_intensity = 0.1f;
    vortf.generate();
    int nframes = 100;
    for( int i = 0; i < nframes; i++ ) {
        float t = ( 1.0f * i ) / nframes;
        cout << "frame " << i << "\n";
        std::ostringstream s;
        s << "frames/hk_turb_" << std::setfill('0') << std::setw(4) << i << ".jpg";
        std::string filename;
        filename = s.str(); 
        //vortex vort;
        vf.position_fill();
        vf.apply( vortf, t );
        //vf.vortex( vort );
        //vf.concentric();
        //cout << "testing index \n";
        //auto v =  vf.index( { 5, 5 } );
        //cout << v.x << " " << v.y << "\n";
        b.warp( a, vf, 1.0f, true, true, SAMP_REFLECT );
        b.write_jpg( filename, 100 ); 
        //vector_field vf10( { 10, 10 } );
        //vf10.turbulent( vortf );
        //vf10.vortex( vort );
        //vf10.concentric();
        //b.warp( a, vf10, 1.0f, true, true, SAMP_REPEAT );
        //b.write_jpg( "hk_warp10.jpg", 100 );
    }
}

void test_splat() {
    fimage a;
    a.load( "../../Jen-C/hk_square.jpg" ); 
    a *= 0.5f;
    fimage splat;
    splat.load( "../../Jen-C/orb.jpg" ); 

    std::optional< std::reference_wrapper< image< frgb  > > > mask; 
    std::optional< std::reference_wrapper< frgb > > tint; 
   
    a.splat( 
        { 0.0f, 0.0f }, 			    // coordinates of splat center
        0.3f, 			                // radius of splat
        0.0f, 			                // rotation in degrees
        splat,
        mask,
        tint 	);	                    // image of the splat  

    frgb my_tint =  { 0.5f, 0.5f, 0.5f };
    tint = std::ref( my_tint );
    a.splat( 
        { 0.3, 0.4f }, 			        // coordinates of splat center
        0.2f, 			                // radius of splat
        60.0f, 			                // rotation in degrees
        splat,
        mask,
        tint 	);	                    // image of the splat  

    a.splat( 
        { 0.4, 0.3f }, 			        // coordinates of splat center
        0.2f, 			                // radius of splat
        -30.0f, 			            // rotation in degrees
        splat,
        mask,
        tint 	);	                    // image of the splat  

    my_tint = { 1.0f, 0.5f, 0.0f }; 
    a.splat( 
        { 1.0f, -1.0f }, 			    // coordinates of splat center
        0.8f, 			                // radius of splat
        180.0f, 			            // rotation in degrees
        splat,
        mask,
        tint 	);	                    // image of the splat  

    a.write_jpg( "hk_splat.jpg", 100 );
}
/*
void test_cluster() {
    fimage a;
    a.load( "../../Jen-C/hk_square.jpg" ); 
    a *= 0.5;
    vector_field vf( a.get_dim() );
    vortex vort;
    vf.vortex( vort );
    //vf += { 0.5f, 0.0f };   // add wind
    vf.normalize();
    fimage splat;
    splat.load( "../../Jen-C/orb.jpg" ); 

    element el( { -0.1f, 0.0f },    // position
            0.1f,                    // scale
            0.0f,
            0.0f,
            splat );
    next_element next_elem( 1000, a.get_bounds() );
    advect_element advector( vf, 1.4f );  
    //scale_ratio shrinker( 0.99f ); 
    angle_branch brancher( 100 );                 
    next_elem.add_function( advector );
    next_elem.add_function( brancher );
    cluster cl( el, next_elem, 100, 0, 10, 0.001f, a.get_bounds() );
    cl.render( a );
    a.write_jpg( "hk_cluster.jpg", 100 );                    
}
*/

void test_branch() {
    const int nframes = 100;
    using std::cout;

    fimage a;
    a.load( "../../Jen-C/hk_square.jpg" );
    a *= 0.5;
    vector_field vf( a.get_dim() );
    vf.fill( { 1.0f, 0.0f } );   // add wind
    vf.normalize();
    fimage splat;
    splat.load( "../../Jen-C/orb.jpg" );

    element el( { -0.1f, 0.0f },    // position
            0.05f,                    // scale
            0.0f,
            0.0f,
            splat );

    next_element next_elem( 100, a.get_bounds() );
    advect_element advector( vf, 1.4f );  
    ratio< float > shrinker( 0.9f ); 
    scale_fn scaler( shrinker );
    angle_branch brancher( 4, 1 ); 
    curly curler( 0.5f );

    wiggle wiggler1( 0.25f, 0.3f, 0.0f, 0.0f );
    time_param wiggle_time1( wiggler1 );
    adder< float > add_wiggle1;
    add_wiggle1.r.add_function( wiggle_time1 );
    curler.curliness.add_function( add_wiggle1 );

    wiggle wiggler( 4.0f, 45.0f, 0.0f, 10.0f );
    log_fn wiggle_damper( -40.0f, 1.0f );
    scale_param wiggle_damper_param( wiggle_damper );
    wiggler.amplitude.add_function( wiggle_damper_param );
    index_param wiggle_indexer( wiggler );
    adder< float > add_wiggle;
    add_wiggle.r.add_function( wiggle_indexer );
    orientation_fn orientation_wave( add_wiggle );

    next_elem.add_function( scaler );
    next_elem.add_function( curler );
    next_elem.add_function( orientation_wave );
    next_elem.add_function( advector );
    next_elem.add_function( brancher );

    cluster cl( el, next_elem, 100, 0, 10, 0.001f, a.get_bounds() );
    float scale = 0.0f;
    for( int i = 0; i < nframes; i++ ) {
        float t = ( 1.0f * i ) / nframes;
        cout << "frame " << i << "\n";
        std::ostringstream s;
        s << "frames/hk_branch_" << std::setfill('0') << std::setw(4) << i << ".jpg";
        std::string filename;
        filename = s.str();
        scale += 0.00175f;
        cl.root_elem.scale = scale;

        // swim
        element el2 = cl.root_elem;
        vec2f position = el2.position;
        any_image img( a );
        element_context ctxt( el2, cl, img, t );
        next_elem( ctxt );
        vec2f delta = position - el2.position;
        cl.root_elem.position += delta * 0.075f;

        fimage b( a );
        cl.render( b, t );
        b.write_jpg( filename, 100 );
    }                  
}

/*void test_melt() {
    const int nframes = 100;
    using std::cout;
    cout << "\nTESTING MELT\n\n";

    fimage a;
    a.load( "../../Jen-C/hk_square.jpg" ); 

    //vector_field vf( a.get_dim() );
    vortex_field vort_field( 10, false ); */
    //vf.apply( vort_field );

/*
    eff_vector_warp< frgb > warper( vf, 0.025f, false, true, SAMP_REPEAT );
    typedef std::function< bool ( buffer_pair< frgb >&, const float& ) > eff_fn_frgb;
    eff_fn_frgb warp_fn( warper );
    iterative_melt< frgb > melter( warp_fn, a ); 

    for( int i = 0; i < nframes; i++ ) {
        cout << "frame " << i << "\n";
        std::ostringstream s;
        s << "hk_iter_" << std::setfill('0') << std::setw(4) << i << ".jpg";
        std::string filename;
        filename = s.str();
        ((fimage &)(melter())).write_jpg( filename, 100 );
        melter.iterate();
    }

    vector_melt< frgb > vmelt( vf, 0.025f ); 
    buffer_pair< frgb > buf( a );

    for( int i = 0; i < nframes; i++ ) {
        cout << "frame " << i << "\n";
        std::ostringstream s;
        s << "frames/hk_vect_" << std::setfill('0') << std::setw(4) << i << ".jpg";
        std::string filename;
        filename = s.str();
        buf.reset( a );
        vmelt( buf );
        ((fimage &)(buf())).write_jpg( filename, 100 );
        vmelt.iterate();
    }
*/ /*
    vector_fn vort_fn = vort_field;
    functional_melt< frgb > fmelt( vort_fn, 0.025f, a.get_dim() );
    buffer_pair< frgb > buf( a );

    for( int i = 0; i < nframes; i++ ) {
        cout << "frame " << i << "\n";
        std::ostringstream s;
        s << "frames/hk_func_" << std::setfill('0') << std::setw(4) << i << ".jpg";
        std::string filename;
        filename = s.str();
        buf.reset( a );
        fmelt( buf );
        ((fimage &)(buf())).write_jpg( filename, 100 );
        fmelt.iterate();
    }
}
*/
void circle_crop() {
    fimage a;
    a.load( "../../Jen-C/RainbowMoon.jpg" ); 
    a.circle_crop( 0.15f );
    a.write_jpg( "moonsplat.jpg", 100 );
}

void test_mask() {
    fimage a;
    a.load( "../../Jen-C/galaxy.jpg" ); 
    fimage splat;
    splat.load( "moonsplat.jpg");
    fimage mask;
    mask.load( "moonmask.jpg");
    frgb tint = { 0.5f, 0.5f, 0.5f };

    a.splat( 
        { 0.3f, -0.3f }, 			    // coordinates of splat center
        0.6f, 			                // radius of splat
        0.0f, 			                // rotation in degrees
        splat,                          // image of the splat
        mask,
        tint,
        MASK_ADDITIVE
    );	                      

    a.write_jpg( "galaxy_moon.jpg", 100 );
}

int main() {
    //test_frgb();
    //test_vect2();
    //test_fimage();
    //test_uimage();
    //test_ucolor(); 
    //test_vector_field();
    //test_splat(); 
    //test_cluster();
    //test_branch();
    //test_melt();
    //circle_crop();
    test_mask();

    return 0;
}

template struct iterative_melt< frgb >;
template struct eff_vector_warp< frgb >;