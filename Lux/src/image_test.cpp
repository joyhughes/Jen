#include <string>
#include "any_image.hpp"
#include "uimage.hpp"
#include "fimage.hpp"
#include "buffer_pair.hpp"
#include "scene.hpp"
#include <map>

void splat_test() {
    //uimage img( vec2i( 512, 512 ) );
    uimage img1( "../diffuser_files/hk_square_512.jpg" );
    uimage img2( vec2i( 512, 512 ) );
    uimage splat;
    splat.load( "../diffuser_files/circle.jpg" );
    std::cout << "image size = " << img1.size() << "splat size = " << splat.size() << std::endl;
    //splat.fill( 0xffffffff );
    //splat.crop_circle();
    img1.splat( splat, false, { 0.0, 0.0 }, 0.3, 0.0 );
    img1.write_jpg( "../samples/element_test.jpg", 100 );
    std::cout << "--------------------------------------";
    img2.splat( splat, false, { 0.0, 0.0 }, 0.3, 0.0 );
    img2.write_jpg( "../samples/element_test2.jpg", 100 );
}
/*
// Unit test for splat_element()
void element_test() {
    fbuf_ptr img   = std::make_shared< buffer_pair< frgb > >( "../samples/mando.jpg" );
    any_buffer_pair_ptr splat = std::make_shared< buffer_pair< frgb > >( "../diffuser_files/circle.jpg" );
    element el;
    el.img = splat;
    el.mask = splat;
    el.scale = 0.3;
    next_element ne;
    splat_element( img, el );
    img->get_image().write_jpg( "../samples/element_test.jpg", 100 );
}
*/

void scene_test() {
    scene s;
    std::map< std::string, std::string > elem_img_bufs, elem_mask_bufs; // Handle forward references to element buffers

    s.elements[ "elem" ] = std::make_shared< element >();
    elem_img_bufs[ "elem" ] = "splat";
    s.buffers[ "img" ]   = std::make_shared< buffer_pair< ucolor > >( "../diffuser_files/hk_square_512.jpg" );
    s.buffers[ "splat" ] = std::make_shared< buffer_pair< ucolor > >( "../diffuser_files/circle.jpg" );
    element& el = *s.elements[ "elem" ];
    //el.img = s.buffers[ "splat" ];
    for( auto& e : elem_img_bufs  ) { s.elements[ e.first ]->img  = s.buffers[ e.second ]; std::cout << "element " << e.first << " img buffer set to "  << e.second << std::endl; }

    next_element ne;
    cluster cl( el, ne );
    //splat.fill( 0xffffffff );
    //splat.crop_circle();
    element_context ctx( el, cl, s, s.buffers[ "img" ] );
    el( s.buffers[ "img" ], ctx );
    //img.splat( { 0.0, 0.0 }, 0.5, 0.0, splat );
    std::get< ubuf_ptr >( s.buffers[ "img" ] )->get_image().write_jpg( "../samples/element_test.jpg", 100 );
}

void read_write_image(const std::string& prefix) {
    // Load the initial image from the given filename
    std::string input_filename = prefix + ".jpg";
    uimage img;
    img.load(input_filename);
    img.mip_it();
    //img.dump();
    //img.fill( 0xffffffff );
    //img.crop_circle();

    // Save the results into files with related but distinct names
    for( int level = 0; level < img.get_mip_levels(); level++ ) {
        std::string output_filename = prefix + std::to_string(level) + ".jpg";
        img.write_jpg(output_filename, 100, level);
    }
}

void process_image(const std::string& prefix) {
    // Load the initial image from the given filename
    std::string input_filename = prefix + ".jpg";
    uimage img(input_filename);

    // Test move constructor
    uimage img2(std::move(img));

    // Save the results into files with related but distinct names
    std::string output_filename = prefix + "_move_constructor.jpg";
    img2.write_jpg(output_filename, 100);

    // Test copy assignment operator
    uimage img3;
    img3 = img2;

    output_filename = prefix + "_copy_assignment.jpg";
    img3.write_jpg(output_filename, 100);

    // Test move assignment operator
    uimage img4;
    img4 = std::move(img3);

    output_filename = prefix + "_move_assignment.jpg";
    img4.write_jpg(output_filename, 100);
}

void copy_any_buffer(const std::string& prefix) {
    std::string input_filename = prefix + ".jpg";
    ubuf_ptr buf1 = std::make_shared< buffer_pair< ucolor > >( input_filename ); 
    any_buffer_pair_ptr any_buf1 = buf1;
    ubuf_ptr buf2 = std::make_shared< buffer_pair< ucolor > >(/* vec2i( 512, 512 ) */);
    any_buffer_pair_ptr any_buf2 = buf2;
    //std::cout << "preparing to copy buffer" << std::endl;
    std::visit( [ &, buf1 ]( auto& b ) { copy_buffer( b, buf1 ); }, any_buf2 );
    //std::cout << "buffer copied" << std::endl;

    uimage img2 = buf2->get_image();
    std::string output_filename = prefix + "_copy_any_buffer.jpg";
    img2.write_jpg(output_filename, 100);

    uimage img1( input_filename );
    output_filename = prefix + "_copy_any_buffer_original.jpg";
    img1.write_jpg(output_filename, 100);
}

int main(int argc, char* argv[]) {
    
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_filename>" << std::endl;
        return 1;
    }

    std::string input_filename = argv[1]; 

    read_write_image(input_filename);
    //copy_any_buffer( input_filename );
    //splat_test();

    return 0;
}

template class image< ucolor >;     // uimage