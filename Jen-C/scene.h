typedef struct element element;
typedef struct element_list element_list;
typedef struct cluster cluster;
typedef struct cluster_list cluster_list;

struct element_list {
	int n;
	element *elems;
	char name[255];
};

struct element
{
	char name[ 255 ];
	fimage splat;
	bool use_mask;
	fimage mask;		// Three channel floating point alpha
	bool use_warp;
	func_tree warp;		// Function for warping can depend on cluster generative function results
};

struct cluster_list {
	int n;
	cluster *clusters;
	char name[255];
};

// Specifies a grouping of subjects with various generative rules
// Avoids functions with a bazillion arguments
struct cluster
{
	char name[ 255 ];

	int n;						// Number of elements in cluster ( subject to change )

	bool  bounded;				// Is there a boundary condition?
	vect2 bound_min,bound_max;	// Bounding rectangle for cluster

	element *elem;				// maybe: replace with multiple possible elements

	bool tlc;					// top level cluster?

	bool  color_filter;			// Filter the splat color?
	// future: palette in generative function
	palette *color_palette;

	// generative function
	func_tree gen_func;			// Generative function of cluster

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
	bool reflect_x, reflect_y; 	// Future: change to warp
	int  x_mirror, y_mirror;
	char output_basename[255];
	bool use_wave;				// Use composed wave for aurora
	bool use_perturb;
	int perturb_steps;

	vect2 bound_min, bound_max;	// Bounding rectangle for scene

	// images
	fimage base_fimg, result_fimg, overlay_fimg, mask_fimg, stencil_fimg;

	element_list 	scene_elems;
	func_tree_list	scene_funcs;
	cluster_list 	clusters;				// Top level clusters
	
} scene;

// ********************** Element functions ********************** 

void element_init( element *elem, const char *name );
bool element_load( element *elem, FILE *fp, char *junk );

// ********************** Element list functions ********************** 

void element_list_init( element_list *elist, int n, const char *name );
element *element_list_index( element_list *elist, const char *name );

// ********************** Cluster functions ********************** 

void cluster_init( cluster *k );
//void cluster_copy( cluster *in, cluster *out );
bool cluster_load( cluster *k, scene *scn, FILE *fp, char *junk );

void cluster_set_gen_func( cluster *k, func_tree *gen_func );
bool cluster_set_leaf( cluster *uck, const char *leaf_name, func_data leaf_val );
void cluster_set_bounds( cluster *uck, vect2 bmin, vect2 bmax);
void cluster_set_n( cluster *k, int n );
void cluster_set_name( cluster *k, const char *name );
void cluster_set_element( cluster *k, element *elem );
//void cluster_set_run( int n, vect2 start, float step, float prop, float ang_offset, cluster *uck );
//void cluster_set_color( palette *color_palette, float start_color, float color_inc, float brightness, float brightness_prop, cluster *uck );
//void cluster_set_size( bool size_prop, float size, cluster *uck );

void cluster_render( cluster *uck, fimage *target_fimg, float time );

// ********************** Cluster List functions ********************** 

void cluster_list_init( cluster_list *clist, int n, const char *name);
void cluster_list_copy( cluster_list *in, cluster_list *out );

// ********************** Scene functions ********************** 

void scene_init( scene *scn );
void scene_free( scene *scn );
int scene_load( scene *scn, const char *filename );
void scene_render( float time, scene *scn );





