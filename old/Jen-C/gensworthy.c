#include ./jen.h
#include ./flow.h

// "runs" are clusters of subjects advected through vector field
// run can continue a certain number of iterations or until crossing boundary condition

generate_run( vect2 start, int n, boolean relative_angle, float start_angle, float angle_increment, subspace *s )
{
	vect2 w = start;
	// generate first item

	// generate remaining items
	if (n>1)
	{
		for(i=1; i<n; i++)
		{
			w = advect( w, s->f, 1.0 );	// move through vector field associated with subspace
			// blend through property (size color etc)
			// generate here
		}
	
	}
	
}

// creates a grouping of runs in a circle
generate_circle()

// creates a grouping of runs in a line
generate_line()

main()
{
	vfield f;		// Main vector field for iterating
	vect2 d;		// flow direction
	// for now - test on "squiggle"

	d.x = 1.0; d.y = 0.0;
	// randomly choose vector field

	// dipole, quadrupole, symmetric rotation centers

	// favor symmetry and centering (but not always)

	// randomly choose # of starting points, # of items in run

	// potentially larger item in center

	// more types of objects - less frequent
}