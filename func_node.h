
typedef union func_data func_data;

union func_data {
	bool b;
	int i;
	float f;
	vect2 v;
	frgb c;
	func_data *a;	// Mixed type array - first item is always int and describes the number of remaining items (can be 0)
};

typedef enum {
	FN_BOOL,
	FN_INT,
	FN_FLOAT,
	FN_VECT2,
	FN_FRGB,
	FN_ARRAY,
	FN_NULL
} func_type;

typedef func_data (*gen_func_1)( func_data );

typedef func_data (*gen_func_2)( func_data, func_data );

typedef func_data (*gen_func_3)( func_data, func_data, func_data );

typedef union Gen_func {
	gen_func_1 gf1;
	gen_func_2 gf2;
	gen_func_3 gf3;	
} gen_func;

typedef struct func_node func_node;

struct func_node {

	func_data value;	// Value of leaf or evaluated branch node
	func_type type;
	int nargs;
	gen_func fn;

	bool evaluated;		// Has this node been evaluated?
	bool choose;

	func_node *a;
	func_node *b;
	func_node *c;

	char name[256];

};

typedef struct func_tree func_tree;

struct func_tree {
	int n;
	char name[256];
	func_node *nodes;
};

typedef struct func_tree_list func_tree_list;

struct func_tree_list {
	int n;
	char name[256];
	func_tree *trees;
};

// ********************** Set func_data functions ********************** 

func_data fd_bool( bool b );
func_data fd_int( int i );
func_data fd_float( float f );
func_data fd_vect2( vect2 v );
func_data fd_frgb( frgb c );

// ********************** Evaluation function ********************** 

func_data fnode_eval( func_node *fnode );

// ********************** Sanity check ********************** 

void fnode_print( func_node* fnode );

// ********************** Initialization functions ********************** 

void fnode_init_leaf( func_node* fnode, func_data value, func_type type );						// initialize leaf node 
void fnode_init_1( func_node* fnode, gen_func_1 fn1, func_node *a, func_type type );				// initialize 1 argument node
void fnode_init_2( func_node* fnode, gen_func_2 fn2, func_node *a, func_node *b, func_type type );	// initialize 2 argument node
void fnode_init_3( func_node* fnode, gen_func_3 fn3, func_node *a, func_node *b, func_node *c, func_type type, bool choose);	// initialize 3 argument node

// ********************** identifier function ********************** 

func_type get_gen_func( const char* name, gen_func *gf, int *n, bool *choose );

// ********************** copy function ********************** 

// copies everything but subnode pointers - those are handled in ftree_copy()
//void fnode_copy( const func_node *in, func_node *out );

// ********************** Calculation functions ********************** 
// Organized by type and parameter
// not exposed
// func_data fn_b_copy( func_data f );
// func_data fn_b_even( func_data f1 ); etc...

// ********************** func_tree functions ********************** 

void 		ftree_init( func_tree *ftree, int n );
void 		ftree_set_name( func_tree *ftree, const char *name);
char* 		ftree_get_name( func_tree *ftree);
void 		ftree_load( func_tree *ftree, const char *filename );
func_node* 	ftree_index( func_tree *ftree, const char *name );
void		ftree_print( func_tree *ftree );
void		ftree_reset( func_tree *ftree );
//void		ftree_copy( func_tree *in, func_tree *out );

