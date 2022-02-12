#define TAU 6.283185307179586

#define R x
#define THETA y

typedef struct Vect2 
{

	float x,y;

} vect2;


vect2 v_zero;	// yes, it's a global

float rand1();

float sin_deg( float theta );

float cos_deg( float theta );

float tan_deg( float theta );

float vtoa( vect2 v );

float add_angle( float a1, float a2 );

float v_magnitude( vect2 v );

vect2 v_rotate( vect2 in, float theta);

vect2 v_set( float x, float y );

vect2 v_complement( vect2 v );

vect2 v_normalize( vect2 v );

vect2 v_add( vect2 a, vect2 b );

vect2 v_subtract( vect2 a, vect2 b );

vect2 v_scale( vect2 in, float s );

vect2 v_complex_power( vect2 in, float p );

vect2 v_inverse_square( vect2 in, float diameter, float soften );

// multiplies only x component by b.x
vect2 v_multiply_x( vect2 a,	// padded float
					vect2 b );	// padded float

vect2 v_add_y( 	vect2 a,	// padded float
				vect2 b );	// padded float

// multiplies only y component by b.x
vect2 v_multiply_y( vect2 a,	// padded float
					vect2 b );	// padded float

vect2 box_of_random2( vect2 min, vect2 max);

bool in_bounds( vect2 v, vect2 min, vect2 max, float dist);

vect2 v_radial( vect2 v );

vect2 v_cartesian( vect2 rad );
