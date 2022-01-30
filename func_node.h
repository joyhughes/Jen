typedef vect2 (*vect_func)( vect2, vect2 );

typedef struct func_node func_node;

struct func_node {

	bool leaf;
	vect2 leaf_val;
	vect_func fn;
	func_node *a;
	func_node *b;

};

vect2 fnode_eval( func_node *fnode );

void fnode_init_leaf( func_node* fnode, vect2 leaf_val );

void fnode_init( func_node* fnode, vect_func fn, func_node *a, func_node *b );