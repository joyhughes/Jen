#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>


#define TAU 6.283185307179586



typedef struct Vect2 
{

	float x,y;

} vect2;

typedef vect2 (*vect_func)( vect2, vect2 );

typedef struct func_node func_node;

struct func_node {

	bool leaf;
	vect2 leaf_val;
	vect_func fn;
	func_node *a;
	func_node *b;

};

vect2 v_set( float x, float y )
{
	vect2 w;

	w.x = x; 
	w.y = y;

	return w;
}

vect2 v_add( vect2 a, vect2 b )
{	
	vect2 w;

	w.x = a.x + b.x;
	w.y = a.y + b.y;
	return w;
}

float v_magnitude( vect2 v )
{
	return(sqrt(v.x*v.x + v.y*v.y));
}

// Two argument normalize - second argument ignored
vect2 v_normalize_2( vect2 a, vect2 b )
{
	vect2 w;
	float m = v_magnitude( a );
	
	if(m == 0.0)
	{
		w.x = 0.0;	
		w.y = 0.0;
		
	}
	else
	{
		w.x = a.x / m;
		w.y = a.y / m;
	}
	return w;
}

vect2 fnode_eval( func_node *fnode )
{
	// if( fnode == NULL ) return v_set( 0.0, 0.0 );
	printf("fnode_eval: leaf_val= ( %0.1f, %0.1f )\n", fnode->leaf_val.x, fnode->leaf_val.y );
	if( fnode->leaf ) return fnode->leaf_val;
	else return fnode->fn( fnode_eval( fnode->a ), fnode_eval( fnode->b ) );
}

// Initialized a leaf node
void fnode_init_leaf( func_node* fnode, vect2 leaf_val  )
{
	fnode->leaf = true;
	fnode->leaf_val = leaf_val;
	fnode->fn = NULL;
	fnode->a = NULL;
	fnode->b = NULL;	
}

// Initializes a branch node
void fnode_init(  func_node* fnode, vect_func fn, func_node *a, func_node *b )
{
	fnode->leaf = false;
	fnode->leaf_val = v_set( 0.0, 0.0 );
	fnode->fn = fn;
	fnode->a = a;
	fnode->b = b;	
}

int main( int argc, char *argv[] )
{
	vect2 result;

	func_node fnode1; 
	fnode_init_leaf( &fnode1, v_set( 1.0, 2.0 ) );

	func_node fnode2; 
	fnode_init_leaf( &fnode2, v_set( 2.0, 3.0 ) );

	func_node adder;
	fnode_init( &adder, &v_add, &fnode1, &fnode2 );

	result = fnode_eval( &adder );

	printf( "result = ( %0.1f, %0.1f )\n", result.x, result.y );
	return 0;

} 