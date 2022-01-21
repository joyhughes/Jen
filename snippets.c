	// Where center of sky splats on background
	vect2 sky_center;		sky_center.x = 5.0;				sky_center.y = 6.0;
	float sky_angle;		float sky_angle_min = -20.0;	float sky_angle_max = 20.0;



		sky_angle = sky_angle_min + ( sky_angle_max - sky_angle_min ) / nframes * frame;	// Interpolate sky angle
		fimage_fill( black, &background );
		// splat rotated sky onto background image
		fimage_splat( 
			sky_center, 			// coordinates of splat center
			10.0, 					// radius of splat
			sky_angle, 				// rotation in degrees
			white,					// change the color of splat
			&background, 			// image to be splatted upon
			&sky 					// image of the splat
			); 


// The "jaggie", a colored line with (somewhat) sharp angles
void generate_jaggie( fimage *result, fimage *splat )
{
	vfield f;		// Main vector field for iterating
	vect2 d, c, start, min, max;		// flow direction
	cluster k;
	int i;
	

	// set bounds of vector field larger than image
	min.x =  -1.0; min.y =  -1.0;
	max.x =  11.0; max.y =  11.0;
	vfield_initialize( 1000, 1000, min, max, &f );

	d.x = 1.0; d.y = 0.0;
	vfield_linear( d, &f );
	vfield_add_stripes( &f );
	vfield_normalize( &f );
	start.x = 0.0625; start.y = 4.0;
	cluster_initialize( &k );

	cluster_set_run( 	800, 			// Number of subjects in run
			start, 				// Starting point
			0.0625, 			// Initial step size
			1.0, 				// Step size proportional change per step
			0.0,				// No adjustment to angle
			&k );	

	cluster_set_bounds( f.min, f.max, &k );

	cluster_set_color( 	0.0, 		// Initial color
					0.005, 		// Color increment
					0.5, 		// Initial brightness
					1.0, 		// Brightness proportional change per step
					&k );

	cluster_set_size( true, 4.0, &k  );
	render_cluster( &f, &k, result, splat );
}
