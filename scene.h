
// Specifies a grouping of subjects with various generative rules
// Avoids functions with a bazillion arguments
typedef struct Cluster
{
	// element *id;		// What am I?
	int n;				// Number of subjects in cluster
	vect2 start;		// Location of first item in cluster

	// vector field associated with cluster 
	// vfield *field;

	// Size properties
	float size;			// Size of object in parametric space
	bool  size_prop;	// Is size proportional to step size?

	// Step properties
	float step;			// Initial step size
	float prop;			// Proportional change in stepsize per step
	bool  bounded;		// Is there a boundary condition?
	vect2 bmin,bmax;	// Bounding rectangle for cluster

	// Move at an angle to vector field
	float ang_offset; // Angle of motion relative to vector field
	// float ang_offset_inc;  
	// other angle offset parameters - proportional to size?

	// Parameters for handling angles (in degrees)
	float ang_init;		// initial angle
	float ang_inc;		// angle increment
	bool  ang_relative;	// Are angles relative to vector direction?

	// Color properties
	palette *color_palette;
	bool  color_filter;		// Filter the splat color?
	bool  brightness_ramp;	// ramp brightness up at beginning?
	int   brightness_ramp_length;
	float start_color;		// starting color in palette function
	float color_inc;   		// color change
	float brightness;		// overall brightness of splat
	float brightness_prop; 	// proportional change in brightness per step
	// other brightness change properties here

	// animation properties
	// pulsation etc.
	//

	// branching properties - spawn new clusters recursively
	struct cluster *subclusters;

} cluster;

// Structure to hold all elements of a scene
typedef struct Scene
{
	char base_file[ 255 ];
	bool use_mask;
	char background_file[ 255 ];
	char mask_file[ 255 ];
	char splat_file[ 255 ];		// Near future: will need multiple splat images
	bool use_stencil;
	float stencil_boost;
	char stencil_file[ 255 ];
	bool reflect_x, reflect_y; 	// Future: add kaleidoscope
	int  reflect_x_line, reflect_y_line;
	char output_basename[255];
	bool use_wave;				// Use composed wave for aurora
	bool use_perturb;
	int perturb_steps;

	// image pointers
	fimage *base_fimg, *background_fimg, *mask_fimg, *splat_fimg, *stencil_fimg;
	
} scene;

void cluster_initialize( cluster *k );

void cluster_set_run( int n, vect2 start, float step, float prop, float ang_offset, cluster *uck );

void cluster_set_bounds( vect2 bmin, vect2 bmax, cluster *uck );

void cluster_set_color( palette *color_palette, float start_color, float color_inc, float brightness, float brightness_prop, cluster *uck );

void cluster_set_size( bool size_prop, float size, cluster *uck );

void scene_initialize( scene *scn );

void file_get_string( FILE *fp, char *str, char *junk, bool *eof );

bool file_get_bool( FILE *fp, char *str, char *junk, bool *eof );

int file_get_int( FILE *fp, char *str, char *junk, bool *eof );

float file_get_float( FILE *fp, char *str, char *junk, bool *eof );

void scene_read( const char *filename, scene *scn );

void render_cluster( vfield *vf, cluster *uck, fimage *result, fimage *splat, scene *scn );

void render_cluster_list( int nruns, float ang_offset, cluster *clist, vfield *vf, fimage *splat, fimage *result, scene *scn );

void generate_circle( 	vect2 c, 	// center of circle
						float r, 	// radius of circle
						float start_ang,	// starting angle in degrees
						int m,			// number of runs around circle
						cluster *uck,
						vfield *f );

void generate_grid( vect2 min, vect2 max, int xsteps, int ysteps, float mutation_rate, vfield *f );

void generate_random( int nruns, vect2 imin, vect2 imax, vect2 vmin, vect2 vmax, float ang_offset, cluster *clist);

void generate_aurora( 	float norm_frame, 
						int nruns, 
						vect2 imin, vect2 imax, 
						vect2 start, 
						float ang_offset, 
						wave_params *ang_wave, // wave_params *brightness_wave,
						palette *p,
						vfield *vf, 
						vect_fn_2d_t_params *vortex_field,
						cluster *clist,
						scene *scn );



