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
	int nargs;
	float (*fn)( int, float* );
	func_node **args;
	float *buffer;
};

float fnode_eval( func_node *fnode )
{
	int i;
	func_node **arg;
	float *buf_ptr;

	printf("fnode_eval: leaf_val=%0.1f\n",fnode->leaf_val);
	if( fnode->leaf ) return fnode->leaf_val;
	else {
		buf_ptr = fnode->buffer;
		arg = fnode->args;
		for( i=0; i<fnode->nargs; i++ )
		{
			*buf_ptr = fnode_eval( *arg );
			buf_ptr++;
			arg++;
		}
		return fnode->fn( fnode->nargs, fnode->buffer );
	}
}

float fnode_sum( int nargs, float *args )
{
	int i;
	float sum = 0.0;
	float *arg = args;

	for( i=0; i<nargs; i++ )
	{
		sum += *arg;
		arg++;
	}
	return sum;
}

float fnode_sqrt( int nargs, float *args )
{
	return sqrt( *args );
}

void fnode_init_leaf( func_node* fnode, float leaf_val  )
{
	fnode->leaf = true;
	fnode->leaf_val = leaf_val;
	fnode->nargs = 0;
	fnode->fn = NULL;
	fnode->args = NULL;
	fnode->buffer = NULL;	
}

void fnode_init( func_node* fnode, int nargs )
{
	fnode->leaf = false;
	fnode->leaf_val = 0.0;
	fnode->nargs = nargs;
	fnode->args = (func_node **)malloc( nargs * sizeof( func_node * ) );
	fnode->buffer = (float *)malloc( nargs * sizeof( float ) );
}

int main( int argc, char *argv[] )
{
	int n=10;
	int i;
	func_node leaves[ n ];

	func_node summation;
	fnode_init( &summation, n );
	summation.fn = &fnode_sum;

	func_node *leaf = leaves;
	func_node **arg = summation.args;
	for( i=0; i<n; i++ )
	{
		fnode_init_leaf( leaf, i * 1.0 );
		*arg = leaf;
		arg++;
		leaf++;
	}

	func_node square_root;
	fnode_init( &square_root, 1 );
	square_root.fn = &fnode_sqrt;
	*(square_root.args) = &summation;

	printf( "result = %f\n", fnode_eval( &square_root ) );
	return 0;
}