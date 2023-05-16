#include <string>
#include "image.hpp"
#include "uimage.hpp"

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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_filename>" << std::endl;
        return 1;
    }

    std::string input_filename = argv[1];

    process_image(input_filename );

    return 0;
}

template class image< ucolor >;     // uimage
