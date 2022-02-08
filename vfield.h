typedef struct Vfield
{
	int xdim, ydim;		// Size of image in pixels
	vect2 min, max;		// Bounds of field in cartesian space
	vect2 *f;			// Flow vectors
} vfield;

typedef struct Vfield
{
	int xdim, ydim;		// Size of image in pixels
	vect2 min, max;		// Bounds of field in cartesian space
	vect2 *f;			// Flow vectors
} vfield;

// ********************** Constructor / destructor ********************** 

void vfield_initialize( int xsiz, int ysiz, vect2 fmin, vect2 fmax, vfield *vf );
void vfield_free( vfield *f );

// ********************** Access functions ********************** 

vect2 vfield_index( int xi, int yi, vfield *vf );
vect2 vfield_coord( int xi, int yi, vfield *vf );
vect2 vfield_smooth_index( vect2 v, vfield *vf );
vect2 vfield_advect( vect2 v, vfield *f, float step, float angle);

// ********************** Modification functions ********************** 

void vfield_complement( vfield *f);
void vfield_scale( float s, vfield *f);
void vfield_rotate_vectors( float ang, vfield *f);
void vfield_normalize( vfield *f );
void vfield_inverse_square( float diameter, float soften, vfield *f );

// ********************** Combination functions ********************** 

void vfield_sum( vfield *a, vfield *b, vfield *f);
void vfield_add_complement( vfield *a, vfield *b, vfield *f);
void vfield_add_linear( vect2 v, vfield *f);
void vfield_box( vect2 min, vect2 max, vect2 v, vfield *f );
void vfield_add_stripes( vfield *f );

// ********************** Generation functions ********************** 

void vfield_linear( vect2 v, vfield *f);
void vfield_concentric( vect2 center, vfield *f );
void vfield_rotation( vect2 center, vfield *f );
void vfield_spiral( vect2 center, float cscale, float rscale, vfield *f );
void vfield_turbulent( float diameter, int n, bool random_rotation, vfield *f);














