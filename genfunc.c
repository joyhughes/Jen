// Attempt at expressing generalized funcions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

typedef struct func_node func_node;

struct func_node {

	bool leaf;
	float leaf_val;
	float (*fn)( float, float );
	func_node *a;
	func_node *b;
};

float fnode_eval( func_node *fnode )
{
	printf("fnode_eval: leaf_val=%0.1f\n",fnode->leaf_val);
	if( fnode->leaf ) return fnode->leaf_val;
	else return fnode->fn( fnode_eval( fnode->a ), fnode_eval( fnode->b ) );
}

float fnode_add( float a, float b )
{
	return a + b;
}

void fnode_init_leaf( func_node* fnode, float leaf_val  )
{
	fnode->leaf = true;
	fnode->leaf_val = leaf_val;
	fnode->fn = NULL;
	fnode->a = NULL;
	fnode->b = NULL;	
}

int main( int argc, char *argv[] )
{
	func_node fnode2; 
	fnode_init_leaf( &fnode2, 2.0 );

	func_node fnode3; 
	fnode_init_leaf( &fnode3, 3.0 );

	func_node adder;
	adder.leaf = false;
	adder.leaf_val = 0.0;
	adder.fn = &fnode_add;
	adder.a = &fnode2;
	adder.b = &fnode3;

	printf( "result = %0.1f\n", fnode_eval( &adder ) );
	return 0;
}