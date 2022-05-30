// image_loader wraps low level image I/O routines. Implementation can be changed without affecting image classes.
// Using stb_image, C-style pointer is created when class is initialized with a file name, and freed in the destructor.
// Copy constructor and operator= are disabled to prevent segmentation faults or memory leaks

#include <string>
#include <span>

class image_loader {
    unsigned char *stbi_ptr;
public:
    std :: span< unsigned char > img;
    int xsiz, ysiz, channels;

    // Disable copy constructor and equal operator
    image_loader& operator= (const image_loader&) = delete;
    image_loader(const image_loader&) = delete;   

    image_loader( const std :: string& filename );
    ~image_loader();
};

void wrapped_write_jpg( const std :: string& filename, const int xdim, const int ydim, const int channels, const unsigned char* data, const int quality );