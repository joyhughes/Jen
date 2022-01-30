#define TAU 6.283185307179586

typedef struct Vect2 
{

	float x,y;

} vect2;

vect2 v_zero;	// yes, it's a global

float rand1();

vect2 v_set( float x, float y );

vect2 v_complement( vect2 v );

vect2 v_complement_2( 	vect2 a, 
						vect2 b );	//ignored

vect2 v_magnitude_2( vect2 a,
					vect2 b );	//ignored

vect2 v_normalize( vect2 v );

vect2 v_normalize_2( 	vect2 a, 
						vect2 b );	// ignored

vect2 v_add( vect2 a, vect2 b );

vect2 v_subtract( vect2 a, vect2 b );

vect2 v_scale( vect2 in, float s );

vect2 v_scale_2( vect2 in, 
				vect2 s );		// padded float

vect2 v_inverse_square( vect2 in, float diameter, float soften );

vect2 v_inverse_square_2( 	vect2 in, 
							vect2 soften );	// padded float

vect2 sin_ab( 	vect2 a,	// padded float
				vect2 b );	// padded float

vect2 box_of_random2( vect2 min, vect2 max);

bool in_bounds( vect2 v, vect2 min, vect2 max, float dist);

