#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include "vect2.h"
#include "func_node.h"

// ******************* Fnode functions ******************* 

vect2 fnode_eval( func_node *fnode )
{
	if( fnode == NULL ) return v_zero;
	printf("fnode_eval: leaf_val= ( %0.1f, %0.1f )\n", fnode->leaf_val.x, fnode->leaf_val.y );
	if( fnode->leaf ) return fnode->leaf_val;
	else return fnode->fn( fnode_eval( fnode->a ), fnode_eval( fnode->b ) );
}

// Initialized a leaf node
void fnode_init_leaf( func_node* fnode, vect2 leaf_val )
{
	fnode->leaf = true;
	fnode->leaf_val = leaf_val;
	fnode->fn = NULL;
	fnode->a = NULL;
	fnode->b = NULL;	
}

// Initializes a branch node
void fnode_init( func_node* fnode, vect_func fn, func_node *a, func_node *b )
{
	fnode->leaf = false;
	fnode->leaf_val = v_set( 0.0, 0.0 );
	fnode->fn = fn;
	fnode->a = a;
	fnode->b = b;	
}