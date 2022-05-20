#include <iostream>
#include "linalg.h"
#include "vect2.hpp"
#include "frgb.hpp"

void test_frgb() {
    using std::cout;
    cout << "TESTING FRGB\n\n";
    frgb color( -1.5f, 0.5f, 2.5f );
    cout << "Initial color     " << color << "\n\n";
    color.clamp( -1.0f, 2.0f );
    cout << "Clamped color     " << color << "\n\n";
    color.constrain();
    cout << "Constrained color " << color << "\n\n";
    color.print_SRGB();

    frgb ucolor( (unsigned char)0x00, (unsigned char)0x80, (unsigned char)0xff );
    cout << "\n\nColor from unsigned char " << ucolor << "\n\n";
    ucolor.print_SRGB();
    cout << "\n\n";

    ucolor += color;
    cout << "Color added with += " << ucolor << "\n\n";

    frgb acolor = ucolor + color;
    cout << "Color added with +  " << acolor << "\n\n";

    color[ 0 ] = 0.5;
    cout << "Color after color[ 0 ] = 0.5 " << color << "\n\n";

    float a = color[ 0 ];

    auto it = color.begin();
    it = color.end();
}


void test_vect2() {
    using std::cout;
    using namespace linalg;
    using namespace ostream_overloads;

    cout << "TESTING VECT2\n\n";

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

    bounding_box< float, 2 > bb( vec2f( -1.0f, -1.0f ), vec2f( 1.0f, 1.0f ) );
    vec2f e(  0.5f, 0.5f );
    vec2f f( -1.5f, 1.5f );
    cout << "{ 0.5, 0.5 }  in bounds        " << bb.in_bounds( e )     << "\n";
    cout << "{ -1.5, 1.5 } in bounds        " << bb.in_bounds( f )     << "\n";
    bb.pad( 1.0f );
    cout << "{ 0.5, 0.5 }  in padded bounds " << bb.in_bounds_pad( e ) << "\n";
    cout << "{ -1.5, 1.5 } in padded bounds " << bb.in_bounds_pad( f ) << "\n";

    cout << "\nbox_of_random() test\n";
    for( int i = 0; i < 10; i++ ) { cout << bb.box_of_random() << " "; }
    cout << "\n";
}

int main() {
    test_frgb();
    test_vect2();
    return 0;
}

