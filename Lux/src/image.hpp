// Template base class for rasterized data - used as base for image classes and vector fields
// Handles functions common to all of these classes.
// Supports linear interpolated sampling and mip-mapping - multiple resolutions for anti-aliasing

#ifndef __IMAGE_HPP
#define __IMAGE_HPP

#include "vect2.hpp"
#include "frgb.hpp"
#include "ucolor.hpp"
#include <iterator>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
//#include "any_image.hpp"

typedef enum image_extend
{
	SAMP_SINGLE,
	SAMP_REPEAT,
	SAMP_REFLECT
} image_extend;

typedef enum file_type
{
    FILE_JPG,
    FILE_PNG,
    FILE_BINARY
} file_type;

// Future: To be used for applying noise to images
typedef enum mutation_type {  MUTATE_NONE, 
                              MUTATE_COLOR, 
                              MUTATE_HUE,
                              MUTATE_VALUE,
                              MUTATE_SATURATION 
} mutation_type;

// Kernel options for mip-mapping
typedef enum mip_kernel { MIP_BOX, MIP_TENT, MIP_GAUSSIAN } mip_kernel;

typedef enum direction4 { D4_UP, D4_RIGHT, D4_DOWN, D4_LEFT } direction4; // clockwise
static bool horizontal( direction4 d ) { return ( d == D4_RIGHT || d == D4_LEFT ); }
static bool vertical(   direction4 d ) { return ( d == D4_UP || d == D4_DOWN ); }

typedef enum direction8 { D8_UP, D8_UPRIGHT, D8_RIGHT, D8_DOWNRIGHT, D8_DOWN, D8_DOWNLEFT, D8_LEFT, D8_UPLEFT } direction8;
static bool horizontal( direction8 d ) { return ( d == D8_RIGHT || d == D8_LEFT ); }
static bool vertical(   direction8 d ) { return ( d == D8_UP || d == D8_DOWN ); }
static bool diagonal(   direction8 d ) { return ( (int)d % 2 ); }

template< class T > class image;

// Root template for raster-based data
template< class T > class image {

protected:

    vec2i dim;              // Dimensions in pixels

    // bounding boxes
    bb2f bounds;      // bounding box in linear space
    bb2i ipbounds;    // bounding box in pixel space (int)
    bb2f fpbounds;    // bounding box in pixel space (float)

    // mip-mapping
    bool mip_me;      // Use mip-mapping for this image? Default false.
    bool mipped;      // has mip-map been allocated?
    bool mip_utd;     // is mip-map up to date? Set to false with any modification of base image
    mip_kernel kernel;
    std::vector< std::vector< T > > mip;  // mip-map of image - first level points to base
    //std::vector< T >& base;  // Pixels
    std::vector< T > base;  // Pixels
    std::vector< vec2i > mip_dim;  // dimensions of mip-map levels (int)
    //std::vector< std::unique_ptr< bb2i > > ipbounds_mip;  // pixel space bounding box of mipped image (int)
    //std::vector< std::unique_ptr< bb2f > > fpbounds_mip;  // pixel space bounding box of mipped image (float)
    // resamples image to crate mip-map            
    void de_mip();  // deallocate all mip-maps

public:
    // default constructor - creates empty "stub" image
    image() : dim( { 0, 0 } ), bounds(), ipbounds( { 0, 0 }, { 0, 0 } ), fpbounds( ipbounds ),
        mip_me( false ), mipped( false ), mip_utd( false ), kernel( MIP_TENT )//, base( mip[ 0 ] ) 
        {}     

    // creates image of particular size 
    image( const vec2i& dims ) 
        :  dim( dims ), bounds( { -1.0, ( 1.0 * dim.y ) / dim.x }, { 1.0, ( -1.0 * dim.y ) / dim.x } ), ipbounds( { 0, 0 }, dim ), fpbounds( { 0.0f, 0.0f }, ipbounds.maxv - 1.0f ), 
           mip_me( false ), mipped( false ), mip_utd( false ), kernel( MIP_TENT )//, base( mip[ 0 ] )
            { base.resize( dim.x * dim.y ); }     

    image( const vec2i& dims, const bb2f& bb ) :  dim( dims ), bounds( bb ), ipbounds( { 0, 0 }, dim ), fpbounds( { 0.0f, 0.0f }, ipbounds.maxv - 1.0f ) ,
        mip_me( false ), mipped( false ), mip_utd( false ), kernel( MIP_TENT )//, base( mip[ 0 ] )
            { base.resize( dim.x * dim.y ); }
        
    // copy constructor
    image( const image< T >& img ) : dim( img.dim ), bounds( img.bounds ), ipbounds( img.ipbounds ), fpbounds( ipbounds ), 
        mip_me( img.mip_me ), mipped( false ), mip_utd( false ), kernel( MIP_TENT )//, base( mip[ 0 ] ) 
        {
            std::copy( img.base.begin(), img.base.end(), back_inserter( base ) );
            // copy mip map
            //mip_it();
        }
    
    // resize constructor
    // 
    image( const image< T >& img, const vec2i& max_size ) : dim( max_size ), 
        mip_me( false ), mipped( false ), mip_utd( false ), kernel( MIP_TENT )//, base( mip[ 0 ] ) 
        {
            // set dimensions - fit within max_size rectangle
            if( dim.x > img.dim.x || dim.y > img.dim.y ) {
                float scale = std::min( (float)dim.x / img.dim.x, (float)dim.y / img.dim.y );
                dim = vec2i( (int)( img.dim.x * scale ), (int)( img.dim.y * scale ) );
            }
            bounds = { { -1.0, ( 1.0 * dim.y ) / dim.x }, { 1.0, ( -1.0 * dim.y ) / dim.x } };
            ipbounds = { { 0, 0 }, dim };
            fpbounds = { { 0.0f, 0.0f }, ipbounds.maxv - 1.0f };
            base.resize( dim.x * dim.y );

            //if( !img.mipped ) throw std::runtime_error( "image resize constructor: image to be resized has no mip-map" );
            // splat image into new image (smoothed if mip map is available)
            splat( img, img.mipped );
        }

    // move constructor
    image( image< T >&& img ) : dim( img.dim ), bounds( img.bounds ), ipbounds( img.ipbounds ), fpbounds( ipbounds ), 
        mip_me( img.mip_me ), mipped( img.mipped ), mip_utd( img.mip_utd ), kernel( img.kernel )//, base( mip[ 0 ] ) 
        {
            // move mip map
            mip = std::move( img.mip );
            base = std::move( img.base );
            //mip_it();
        }

    // load constructor
    image( const std::string& filename ) : image() { load( filename ); } 

    friend class vf_tools;  // additional functions for vector fields

    T* get_base() { return &(base[0]); }    

    //typedef std::iterator< std::forward_iterator_tag, std::vector< T > > image_iterator;
    auto begin() noexcept { return base.begin(); }
    const auto begin() const noexcept { return base.begin(); }
    auto end() noexcept { return base.end(); }
    const auto end() const noexcept { return base.end(); }

    void reset();                          // clear memory & set dimensions to zero (mip_me remembered)
    void use_mip( bool m );
    void mip_it();  // mipit good
    const vec2i get_dim() const;
    void set_dim( const vec2i& dims );
    void refresh_bounds(); // calculates default bounding boxes based on pixel dimensions
    const bb2f get_bounds() const;
    const bb2i get_ipbounds() const;
    const bb2f get_fpbounds() const;
    void set_bounds( const bb2f& bb );
    template< class U > bool compare_dims( const image< U >& img ) const { return ( dim == img.get_dim() ); }  // returns true if images have same dimensions

    // Sample base image
    inline void set( const unsigned int& i, const T& val ) { base[ i ] = val; }
    const T index( const vec2i& vi, const image_extend& extend = SAMP_SINGLE ) const;
    inline T index( const unsigned int& i ) const { return base[ i ]; }
    const T sample( const vec2f& v, const bool& smooth = false, const image_extend& extend = SAMP_SINGLE ) const;    
    // Preserved intersting bug       
    const T sample_tile( const vec2f& v, const bool& smooth = false, const image_extend& extend = SAMP_SINGLE ) const;
    // fixed point sample           
    const T sample( const unsigned int mip_level, const unsigned int mip_blend, const vec2i& vi ) const; // mip-map sample using fixed point coordinates

    // sample mip-map
    // const T index( const vec2i& vi, const int& level, const image_extend& extend = SAMP_SINGLE );                
    // const T sample( const vec2f& v, const int& level, const bool& smooth = false, const image_extend& extend = SAMP_SINGLE );           

    // size modification functions
    void resize( vec2i siz );
    void crop( const bb2i& bb );
    void crop_circle( const T& background, const float& ramp_width = 0.0f );	// Sets to zero everything outside of a centered circle
    void crop_circle( const float& ramp_width = 0.0f );	// Sets to zero everything outside of a centered circle
    void mirror(    const image< T >& in,
                    const bool& reflect_x = false, 
                    const bool& reflect_y = true, 
                    const bool& top_to_bottom = true, 
                    const bool& left_to_right = true, 
                    const vec2f& center = vec2f( 0.0f, 0.0f ), 
                    const image_extend& extend = SAMP_SINGLE );
    void turn( const image< T >& in, const direction4& direction );
    void flip( const image< T >& in, const bool& flip_x, const bool& flip_y );
    void resize( const image< T >& in, const vec2i& max_size ); // resize image to fit within max_size rectangle

    // pixel modification functions
    void copy( const image< T >&  img );
    void fill( const T& c );
    void fill( const T& c, const bb2i& bb );
    void fill( const T& c, const bb2f& bb );
    void clear();  // sets all pixels to zero
    void noise( const float& a );
    void noise( const float& a, const bb2i& bb );
    void noise( const float& a, const bb2f& bb );
    
    // functions below use template specialization
    void grayscale() {}
    void clamp( float minc = 1.0f, float maxc = 1.0f ) {}
    void constrain() {}
    void rotate_colors( const int& r ) {}
    void invert() {}
    // fill warp field or offset field values based on vector field - fields should be same size
    void fill( const image< vec2f >& vfield, const bool relative = false, const image_extend extend = SAMP_REPEAT ) {}
    template< class U > inline void advect( int index, image< U >& in, image< U > out ) {} // advect one pixel (warp field and offset field)
    template< class U > void advect( image< U >& in, image< U >& out ) {} // advect entire image (warp field and offset field)

    //void color_noise( const T& minc, const T& maxc, const float& a = 1.0f, const bb2i& );

    // masking
    void apply_mask( const image< T >& layer, const image< T >& mask, const mask_mode& mmode = MASK_BLEND );

    // rendering
    void splat( 
        const image< T >& splat_image,    // image of the splat
        const bool& smooth = false,       // smooth the splat
        const vec2f& center = { 0.0f, 0.0f }, // coordinates of splat center
        const float& scale = 1.0f,  // radius of splat
        const float& theta = 0.0f,  // rotation in degrees
        const std::optional< std::reference_wrapper< image< T > > > mask = std::nullopt,  // optional mask image
        const std::optional< T >& tint = std::nullopt,       // change the color of the splat
        const mask_mode& mmode = MASK_BLEND // how will mask be applied to splat and backround?
    );  

    // warp with vector field
    void warp ( const image< T >& in, 
                const image< vec2f >& vf, 
                const float& step = 1.0f, 
                const bool& smooth = false, 
                const bool& relative = true,
                const image_extend& extend = SAMP_SINGLE );

    // warp with vector function
    void warp ( const image< T >& in, 
                const std::function< vec2f( vec2f ) >& vfn, 
                const float& step = 1.0f, 
                const bool& smooth = false, 
                const bool& relative = true,
                const image_extend& extend = SAMP_SINGLE );

    // warp with warp field
    // can possibly slide by adding or subtracting int value from index
    void warp ( const image< T >& in,
                const image< int >& wf ); 

    // warp with offset field
    void warp ( const image< T >& in,
                const image< vec2i >& of,
                const vec2i &slide = vec2i( 0, 0 ),
                const image_extend& extend    = SAMP_SINGLE,
                const image_extend& of_extend = SAMP_SINGLE );

    // load image from file - JPEG and PNG are only defined for fimage and uimage, so virtual function
    // future - binary file type for any image (needed for vector field and out of range fimage)
    void load( const std::string& filename ) { std::cout << "default image load" << std::endl; }
    void write_jpg( const std::string& filename, int quality ) { std::cout << "default image write_jpg" << std::endl; }
    void write_png( const std::string& filename ) { std::cout << "default image write_png" << std::endl; }
    void read_binary(  const std::string& filename );  
    void write_binary( const std::string& filename );
    // determine file type from extension?
    void write_file( const std::string& filename, file_type ftype = FILE_JPG, int quality = 100 );

    // apply function to each pixel in place (can I make this any parameter list with variadic template?)
    void apply( const std::function< T ( const T&, const float& ) > fn, const float& t = 0.0f );

    // Debugging functions
    void dump() {} // dump image to console
    unsigned int size() { return base.size(); } // return size of image

    // operators
    image< T >&  operator = ( const image< T >&  rhs ); // copy assignment
    image< T >&  operator = ( image< T >&& rhs );      // move assignment
    image< T >&  operator += ( image< T >&  rhs );      // add rhs to this
    image< T >&  operator += ( const T& rhs );
    image< T >&  operator -= ( image< T >&  rhs );
    image< T >&  operator -= ( const T& rhs );
    image< T >&  operator *= ( image< T >&  rhs );
    image< T >&  operator *= ( const T& rhs );
    image< T >&  operator *= ( const float& rhs );
    image< T >&  operator /= ( image< T >&  rhs );
    image< T >&  operator /= ( const T& rhs );
    image< T >&  operator /= ( const float& rhs );

    image< T >&  operator () ();
    //void apply( unary_func< T > func ); // apply function to each pixel (in place)
    //void apply( unary_func< T > func, image< T >& result ); // apply function to each pixel (to target image)
    // apply function to two images
};

#endif // __IMAGE_HPP
