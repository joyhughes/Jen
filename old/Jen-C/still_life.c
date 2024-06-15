#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


typedef struct Coord 
{
	double x,y,z;
} coord;

typedef struct Angle3
{
	double roll,pitch,yaw;
} angle3;

// representaion of camera properties - everything we can specify about physical camera
// At this point don't want to mess with moving camera around
/* typedf struct Camera
{
	coord origin;
	coord aim;
// float exposure
// float fstop
// float ISO
// float focal_distance // in scene units
// float zoom // mm
} */

// structue to represent each type of subject 
typedef struct Component
{
	char *name;
	double rarity;	// probability of generating this component
	int supply;		// number of objects in supply
	int available; 	// number of objects currently available
} component;

// representaion of a generative space in the scene
typedef struct Subspace
{
	int n_comp;		// number of possible components in subspace
	component *clist; 		// list of possible components within the subspace
 	coord min, max;	 		// boundaries of box where components can be located (conceptually a location function)
 	angle3 min_ang, max_ang;	// bounds on potential orientation of object
	// double granularity;		// step size in the space - say 1/4 inch
	// function to generate count of objects
} subspace;

// a realized component within the scene
typedef struct Subject
{
	component *id; // This defines the type of the subject
	coord origin;
	angle3 orientation;
} subject;

// struct scene  // not sure if I'm going to use this just yet
// subspace *slist; // spaces within scene
// 

// returns a random number between 0 and 1
double rand1()
{
	return (rand() * 1.0) / (RAND_MAX * 1.0);
}

coord box_of_random( coord min, coord max)	// I don't know who put it there
{
	coord out;

	out.x = min.x + (max.x - min.x)*rand1();
	out.y = min.y + (max.y - min.y)*rand1();
	out.z = min.z + (max.z - min.z)*rand1();

	return out;
}

angle3 random_angle( angle3 min, angle3 max)
{
	angle3 out;

	out.roll  = min.roll  + ( max.roll  - min.roll  ) * rand1();
	out.pitch = min.pitch + ( max.pitch - min.pitch ) * rand1();
	out.yaw   = min.yaw   + ( max.yaw   - min.yaw   ) * rand1();

	return out;
}

void calculate_rarity(subspace *s)
{
	int i;
	component *comp = s->clist;

	for(i=0;i<s->n_comp;i++)
	{
		comp->rarity = rand1();
		comp ++;
	}
}

void normalize_rarity(subspace *s)
{
	int i;
	double sprob = 0.0;
	component *comp = s->clist;

	// Sum up probabilities
	for(i=0;i<s->n_comp;i++)
	{
		sprob += comp->rarity;
		comp++;
	}

	// Divide probability by sum
	comp = s->clist;
	for(i=0;i<s->n_comp;i++)
	{
		comp->rarity = comp->rarity / sprob;
		comp++;
	}
}

void print_components(subspace *s)
{
	int i;
	component *comp = s->clist;

	for(i=0;i<s->n_comp;i++)
	{
		printf("%d  %s  %2.2f%%\n", comp->supply, comp->name, comp->rarity*100.0);
		comp++;
	}
}

void add_component(subspace *s, char *name, int supply)
{
	component *comp = &(s->clist[s->n_comp]);	// get pointer to relevant component

	comp->name = (char *)malloc((strlen(name)+2) * sizeof(char)); // allocate memory for string
	sprintf(comp->name,"%s",name);
	comp->supply = supply;

	s->n_comp++;
}

void update_availability(subspace *s)
{
	int c;
	component *comp = s->clist;

	for(c=0; c < s->n_comp; c++)
	{
		comp->available = comp->supply;
		comp++;
	}
}

void print_subject(subject *subj)
{
	printf("%s\n",subj->id->name);
	printf("origin %0.2f %0.2f %0.2f\n", subj->origin.x, subj->origin.y, subj->origin.z);
	printf("orientation  %0.1f %0.1f %0.1f\n\n", subj->orientation.roll, subj->orientation.pitch, subj->orientation.yaw);
}

subject generate( subspace *s )
{
	int c;
	subject subj;
	double prob = 0.0;
	double sprob = 0.0;
	double r;	// random number
	component *comp = s->clist;

	// Determine the total probability of available components
	for(c=0; c < s->n_comp; c++)
	{
		if(comp->available > 0 ) 
		{
			sprob += comp->rarity;
		}
		comp++;
	}

	// If no more components are available, NULL result
	if(sprob == 0.0)
	{
		printf("Error - no available component\n");
		subj.id = NULL;
		return subj;
	}

	// Choose random component from available in subspace list 
	comp = s->clist;
	r = sprob * rand1();
	while( r > prob )
	{
		if(comp->available) 
		{
			prob += comp->rarity;
			subj.id = comp;
		}
		comp++;
	}

	// Calculate random position and orientation
	subj.origin      = box_of_random( s->min ,    s->max     );
	subj.orientation = random_angle ( s->min_ang, s->max_ang );

	return subj;
}

void generate_cluster( subspace *s, subject *subj, int n)
{
	int i = 0;
	int j, new, maxnew;
	while(i<n)
	{
		subj[i] = generate(s);
		maxnew = subj[i].id->available;		// maximum new is constrained both by supply and total number in subspace
		if( maxnew > n-i ) maxnew = n-i;	
		subj[i].id->available--;
		i++;
		if( maxnew > 1)	// possibly create cluster of items
		{
			new = (rand() % maxnew) + 1;
			if( new > 1 )
			{
				for(j=1;j<new;j++)	// create cluster of subjects in same location with separate angles
				{
					subj[i] = subj[i-1];
					// Possibly perturb position and angle here
					subj[i].orientation = random_angle( s->min_ang, s->max_ang);
					subj[i].id->available--;
					i++;
				}
			}
		}
	}
}

int main( int argc, char *argv[] ) 
{
	// first argument - generative space file
	// second argument - number of scenes generated
	// may just generate on the fly for now
	int nscenes = 2; // just generate 2 scenes right now for vibe / reality check

	// camera cam;
	// light lgt;
	component vase_comps[1024];
	component contents_comps[1024];
	component loose_comps[1024];
	subspace vases, contents, loose;
	subject vase;
	subject contents_subj[7];
	subject loose_subj[7];
	int n_contents,n_loose;					// Number of items in vase, number of loose items
	coord cammin, cammax;					// Camera subspace bounds
	int n,i;								// Counters
	int maxnew;								// maximum number of new subjects in cluster
	time_t t;

	/*/ // Initialize camera box
	cammin.x   = -6.0;	cammin.y   = 24.0;	cammin.z   =  3.0;
	cammax.x   =  6.0;	cammax.y   = 24.0;	cammax.z   = 15.0;
	cam->aim.x =  0.0;	cam->aim.y =  0.0;	cam->aim.x =  0.0;
	*/

	// start random engines
	srand((unsigned) time(&t));

	// Initialize components in each subspace

	// Vases - linear subspace
	vases.clist = vase_comps;
	vases.min_ang.yaw = 0.0;
	vases.min_ang.yaw = 360.0;
	vases.min.x = -6.0;	vases.min.y = 3.0; vases.min.z = 0.0;
	vases.max.x =  6.0;	vases.max.y = 3.0; vases.max.z = 0.0;
	add_component(&vases,"None",1);
	add_component(&vases,"Glass Vase",1);
	add_component(&vases,"Chinese Garlic Jar",1);
	add_component(&vases,"Mason Jar",1);
	add_component(&vases,"Mason Jar with Marbles",1);
	add_component(&vases,"Plaid Vase",1);
	add_component(&vases,"Sugar Bowl",1);
	add_component(&vases,"Mortar and Pestle",1);

	// Vase contents - point subspace dependent on vase position
	contents.clist = contents_comps;
	contents.min_ang.yaw = 0.0;
	contents.min_ang.yaw = 360.0;
	add_component(&contents,"White Feather",3);
	add_component(&contents,"Brown Feather",1);
	add_component(&contents,"Sunflower",1);
	add_component(&contents,"Holly Berries",1);
	add_component(&contents,"Hawthorne Berries",1);
	add_component(&contents,"Bracken Fern",4);
	add_component(&contents,"Statice",7);
	add_component(&contents,"Purple Leaves",1);
	add_component(&contents,"White Berries",1);

	// Loose items - 2D subspace
	loose.clist = loose_comps;
	loose.min_ang.yaw = 0.0;
	loose.min_ang.yaw = 360.0;
	loose.min.x = -8.0; loose.min.y = 3.0; loose.min.z = 0.0;
	loose.max.x =  8.0; loose.min.y = 8.0; loose.min.z = 0.0;

	add_component(&loose,"Mini Apple Duo",2);
	add_component(&loose,"Campbell's Soup",1);
	add_component(&loose,"Mini Pears",5);
	add_component(&loose,"Fuji Apples",3);
	add_component(&loose,"Geode",1);
	add_component(&loose,"Eyeball",1);
	add_component(&loose,"Potato",4);
	add_component(&loose,"Garlic",3);
	add_component(&loose,"Onion",2);

	// calculate rarity of each component & normalize probabilities
	calculate_rarity(&vases);
	calculate_rarity(&contents);
	calculate_rarity(&loose);

	vase_comps[0].rarity = 1.0;	// HACK - probability of no vase always significant

	normalize_rarity(&vases);
	normalize_rarity(&contents);
	normalize_rarity(&loose);

	// important - write file with rarites (otherwise you'll never know!)
	printf("\n%d Types of Vases\n",vases.n_comp);
	print_components(&vases);
	printf("\n%d Types of Vase Contents\n",contents.n_comp);
	print_components(&contents);
	printf("\n%d Types of Loose Items\n",loose.n_comp);
	print_components(&loose);

	// generate each scene
	for(n=1;n<=nscenes;n++) 
	{
		// Reset available supply for each component
		update_availability(&vases);
		update_availability(&contents);
		update_availability(&loose);

		// Camera location
		// cam.origin = box_of_random( cammin, cammax );
		// Light location

		// Other scene properties (backdrop, etc) ... still need to decide on this
		// Choose a vase (or not, vase can be NONE)
		printf("Choosing vase\n");
		vase = generate( &vases );
		contents.min = contents.max = vase.origin;	// All contents at same origin as vase (orientations may vary)

		// Fill the vase with 0,1,3,5,7 subjects
		// If no vase, probability of subjects anyway
		n_contents = (rand()%5) * 2 - 1;
		if(n_contents < 0) n_contents = 0;
		if(n_contents) generate_cluster( &contents, contents_subj, n_contents );

		// Choose subjects for remainder of scene 
		n_loose = (rand()%5) * 2 - 1;
		if(n_loose < 0) n_loose = 0;
		if(n_loose) generate_cluster( &loose, loose_subj, n_loose );

		// Dump whatever's in the scene to the terminal
		printf("\nScene %d\n",n);

		printf("\nVase - ");
		print_subject(&vase);

		printf("\n%d Vase Contents\n",n_contents);
		for(i=0;i<n_contents;i++)
		{
			printf("\nVase Contents %d - ",i+1);
			print_subject(&(contents_subj[i]));
		}

		printf("\n%d Loose Items\n",n_loose);
		for(i=0;i<n_loose;i++)
		{
			printf("\nLoose Item %d - ",i+1);
			print_subject(&(loose_subj[i]));
		}
	}
}

