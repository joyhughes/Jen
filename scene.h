typedef struct element element;

struct element
{
	char name[ 255 ];
	fimage splat;
	fimage mask;		// Three channel floating point alpha
};

typedef struct element_list element_list;

struct element_list {
	int n;
	element *elems;
	char name[255];
};

typedef struct cluster cluster;

// Specifies a grouping of subjects with various generative rules
// Avoids functions with a bazillion arguments
struct cluster
{
	element *id;		// What am I?
	char name[ 255 ];

	int n;				// Number of elements in cluster ( subject to change )


	// vect2 origin;		// Origin point of cluster

	bool  bounded;		// Is there a boundary condition?
	vect2 bmin,bmax;	// Bounding rectangle for cluster

	element *elem;		// maybe: replace with multiple possible elements

	bool tlc;			// top level cluster?

	bool  color_filter;		// Filter the splat color?
	// future: palette in generative function
	palette *color_palette;

	// generative function
	func_tree gen_func;		// necessary? yes.

	func_node *n_elements_leaf;
	func_node *index_leaf;			// keeps track of current iteration of cluster

	func_node *time_leaf;

	func_node *origin_leaf;		
	func_node *position_leaf;		// used recursively to generate
	func_node *position_result;	

	func_node *size_init_leaf;
	func_node *size_leaf;			// used recursively to generate
	func_node *size_result;

	func_node *ang_init_leaf;
	func_node *ang_leaf;			// used recursively to generate
	func_node *ang_result;

	// for now use palette and brightness (could get more sophisticated with color calculation)
	func_node *color_index_init_leaf;
	func_node *color_index_leaf;
	func_node *color_index_result;

	func_node *brightness_init_leaf;
	func_node *brightness_leaf;
	func_node *brightness_result;

	func_node *branch_result;	// boolean result to determine if a subcluster is needed
								// need to determine identity and initial conditions of subcluster

	// branching properties - spawn new clusters recursively
	cluster_list branches;
	func_tree_list branch_functions;

	// Stencil functions?

};

/*	OG cluster properties for reference

	// vector field associated with cluster 
	// vfield *field;

	// Size properties
	float size_init;	// Initial size of object in parametric space
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
	bool  brightness_ramp;	// ramp brightness up at beginning?
	int   brightness_ramp_length;
	float color_index_init;	// starting color in palette function
	float color_inc;   		// color change

	float brightness_init;		// overall brightness of splat
	float brightness_prop; 	// proportional change in brightness per step
	// other brightness change properties here

	// animation properties
	// pulsation etc.
	//
	*/

typedef struct cluster_list cluster_list;

struct cluster_list {
	int n;
	cluster *clusters;
	char name[255];
};

// Structure to hold all elements of a scene
typedef struct Scene
{
	char name[ 255 ];

	char base_file[ 255 ];
	bool use_mask;
	char overlay_file[ 255 ];
	char mask_file[ 255 ];
	bool use_stencil;
	float stencil_boost;
	char stencil_file[ 255 ];
	bool reflect_x, reflect_y; 	// Future: add kaleidoscope
	int  reflect_x_line, reflect_y_line;
	char output_basename[255];
	bool use_wave;				// Use composed wave for aurora
	bool use_perturb;
	int perturb_steps;

	// images
	fimage base_fimg, result_fimg, overlay_fimg, mask_fimg, stencil_fimg;

	element_list 	scene_elems;
	func_tree_list	scene_funcs;
	cluster_list 	clusters;				// Top level clusters
	
} scene;

// ********************** Cluster functions ********************** 

void cluster_initialize( cluster *k );

void cluster_copy( cluster *in, cluster *out );

bool cluster_set_leaf( cluster *uck, const char *leaf_name, func_data leaf_val );

void cluster_set_bounds( vect2 bmin, vect2 bmax, cluster *uck );

//void cluster_set_run( int n, vect2 start, float step, float prop, float ang_offset, cluster *uck );

//void cluster_set_color( palette *color_palette, float start_color, float color_inc, float brightness, float brightness_prop, cluster *uck );

//void cluster_set_size( bool size_prop, float size, cluster *uck );

void cluster_render( vfield *vf, cluster *uck, fimage *result, fimage *splat, scene *scn );

// ********************** Cluster List functions ********************** 

void cluster_list_init( cluster_list *clist, int n, const char *name);

void cluster_list_copy( cluster_list *in, cluster_list *out );

// ********************** Scene functions ********************** 

void scene_initialize( scene *scn );

void scene_free( scene *scn );

void scene_load( const char *filename, scene *scn );

void scene_render( float time, scene *scn );





