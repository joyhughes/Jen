

typedef union Func_data {
	bool b;
	int i;
	float f;
	vect2 v;
	frgb c;
} func_data;

typedef func_data (*gen_func_1)( func_data );

typedef func_data (*gen_func_2)( func_data, func_data );

typedef struct func_node func_node;

struct func_node {

	func_data leaf_val;	// Zero arguments - leaf
	gen_func_1 fn1;		// One argument
	gen_func_2 fn2;		// Two arguments
	func_node *a;
	func_node *b;
	char name[256];

};

typedef struct func_tree func_tree;

struct func_tree {
	int n;
	func_node *nodes;
	char name[256];
};

// ********************** Set func_data functions ********************** 

func_data fd_bool( bool b );
func_data fd_int( int i );
func_data fd_float( float f );
func_data fd_vect2( vect2 v );
func_data fd_frgb( frgb c );

// ********************** Evaluation function ********************** 

func_data fnode_eval( const func_node *fnode );

// ********************** Initialization functions ********************** 

void fnode_init_leaf( func_node* fnode, func_data leaf_val );						// initialize leaf node 
void fnode_init_1( func_node* fnode, gen_func_1 fn1, func_node *a );				// initialize 1 argument node
void fnode_init_2( func_node* fnode, gen_func_2 fn2, func_node *a, func_node *b );	// initialize 2 argument node

// ********************** float ( float, float ) ********************** 

func_data fn_f_multiply( func_data f1, func_data f2 );
func_data fn_f_sin_ab( func_data f1, func_data f2 );

// ********************** vect2 ( vect2 ) ********************** 

func_data fn_v_radial( func_data v );
func_data fn_v_cartesian( func_data v );

// ********************** vect2 ( vect2, float ) ********************** 

func_data fn_v_add_y( func_data v, func_data f );
func_data fn_v_multiply_y( func_data v, func_data f );

// ********************** func_tree functions ********************** 
/*
void ftree_init( func_tree *ftree, int n );
void ftree_set_name( func_tree *ftree, const char *name);
char* ftree_get_name( func_tree *ftree);
void ftree_load( func_tree *ftree, const char *filename );
fnode* ftree_index( func_tree *ftree, const char *name );
*/
