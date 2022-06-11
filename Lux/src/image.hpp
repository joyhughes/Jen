// Template base class for rasterized data - used as base for image classes and vector fields
// Handles functions common to all of these classes.
// Supports linear interpolated sampling and mip-mapping - multiple resolutions for anti-aliasing

#ifndef __IMAGE_HPP
#define __IMAGE_HPP

#include "vect2.hpp"
#include "frgb.hpp"
#include "ucolor.hpp"
#include <vector>

enum image_extend
{
	SAMP_SINGLE,
	SAMP_REPEAT,
	SAMP_REFLECT
};
typedef enum image_extend image_extend;

// Root template for raster-based data
template< class T > class image {

protected:
    typedef image< T > I;

    vec2i dim;              // Dimensions in pixels
    std :: vector< T > base;  // Pixels

    // bounding boxes
    bb2f bounds;      // bounding box in linear space
    bb2i ipbounds;    // bounding box in pixel space (int)
    bb2f fpbounds;    // bounding box in pixel space (float)

    // mip-mapping
    bool mip_me;      // Use mip-mapping for this image? Default false.
    bool mipped;      // has mip-map been allocated?
    bool mip_utd;     // is mip-map up to date? Set to false with any modification of base image
    std :: vector< std :: unique_ptr< std :: vector< T > > > mip;  // mip-map of image
    std :: vector< std :: unique_ptr< bb2i > > ipbounds_mip;  // pixel space bounding box of mipped image (int)
    std :: vector< std :: unique_ptr< bb2f > > fpbounds_mip;  // pixel space bounding box of mipped image (float)
    // resamples image to crate mip-map            
    void mip_it();  // mipit good
    void de_mip();  // deallocate all mip-maps

public:
    // default constructor - creates empty "stub" image
    image() : dim( { 0, 0 } ), bounds(), ipbounds( { 0, 0 }, { 0, 0 } ), fpbounds( ipbounds ),
        mip_me( false ), mipped( false ), mip_utd( false ) {}     
    // creates image of particular size 
    image( const vec2i& dims ) 
        :  dim( dims ), bounds( { -1.0, ( 1.0 * dim.y ) / dim.x }, { 1.0, ( -1.0 * dim.y ) / dim.x } ), ipbounds( { 0, 0 }, dim ), fpbounds( ipbounds ), 
           mip_me( false ), mipped( false ), mip_utd( false ) {}     
    image( const vec2i& dims, const bb2f& bb ) :  dim( dims ), bounds( bb ), ipbounds( { 0, 0 }, dim ), fpbounds( ipbounds ) ,
        mip_me( false ), mipped( false ), mip_utd( false ) {}     
    // copy constructor
    image( const I& img ) : dim( img.dim ), bounds( img.bounds ), ipbounds( img.ipbounds ), fpbounds( ipbounds ), 
        mip_me( img.mip_me ), mipped( img.mipped ), mip_utd( img.mip_utd ) 
        {
            std :: copy( img.base.begin(), img.base.end(), back_inserter( base ) );
            mip_it();
        }     

    void reset();                          // clear memory & set dimensions to zero (mip_me remembered)
    void use_mip( bool m );
    vec2i get_dim();
    void set_dim( const vec2i& dims );
    bb2f get_bounds();
    void set_bounds( const bb2f& bb );
    template< class U > bool compare_dims( const image< U >& img ) { return ( dim == img.dim ); }  // returns true if images have same dimensions

    // Sample base image
    T index( const vec2i& vi, const image_extend& extend = SAMP_SINGLE );
    T sample( const vec2f& v, const image_extend& extend = SAMP_SINGLE );           // quick and dirty sampling of nearest pixel value
    T smooth_sample( const vec2f& v, const image_extend& extend = SAMP_SINGLE );    // linearly interpolated between corner values

    // sample mip-map
    T index( const vec2i& vi, const int& level, const image_extend& extend = SAMP_SINGLE );                
    T sample( const vec2f& v, const int& level, const image_extend& extend = SAMP_SINGLE );           // quick and dirty sampling of nearest pixel value
    T smooth_sample( const vec2f& v, const float& level, const image_extend& extend = SAMP_SINGLE );  // linearly interpolated between corner values and mip-map levels

    // size modification functions
    void resize( vec2i siz );
    void crop( const bb2i& bb );
    void circle_crop( float ramp_width = 0.0f );	// Colors black everything outside of a centered circle

    // pixel modification functions
    //void grayscale();
    void fill( const T& c );

    // masking
    void apply_mask( const I& mask, I& result );

    // rendering
    void splat( 
        const vec2f& center, 			// coordinates of splat center
        const float& scale, 			// radius of splat
        const float& theta, 			// rotation in degrees
        const T& tint,				    // change the color of splat
        const I& g 			            // image of the splat
	);

    // operators
    I& operator += ( I& rhs );
    I& operator += ( const T& rhs );
    I& operator -= ( I& rhs );
    I& operator -= ( const T& rhs );
    I& operator *= ( I& rhs );
    I& operator *= ( const T& rhs );
    I& operator *= ( const float& rhs );
    I& operator /= ( I& rhs );
    I& operator /= ( const T& rhs );
    I& operator /= ( const float& rhs );

    //void apply( unary_func< T > func ); // apply function to each pixel (in place)
    //void apply( unary_func< T > func, image< T >& result ); // apply function to each pixel (to target image)
    // apply function to two images
};


//typedef image< frgb >           fimage;
typedef image< vec2f >          vfield;
// typedef image< ucolor >  ulimage;
// packed unsigned char image?

#endif // __IMAGE_HPP
