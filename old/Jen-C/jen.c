// jen.c contains snippets of code that generate various kinds of clusters. 
// While this code has been deprecated in favor of using .jen files, it can serve as a guide and possibly be reused

// creates a grouping of runs evenly spaced in a circle
// occasional mutation changing some aspect of a run
// needs to be fixed
void generate_circle( 	vect2 c, 	// center of circle
						float r, 	// radius of circle
						float start_ang,	// starting angle in degrees
						int m,			// number of runs around circle
						cluster *uck,
						vfield *f )
{
	int i;
	float theta = TAU * start_ang / 360.0;

	for(i=0; i<m; i++)
	{
		uck->start.x = c.x + r * cos(theta);
		uck->start.y = c.y + r * sin(theta);
		//generate_cluster( f, uck, );
		theta += TAU / m;
	}
}

// generates a cluster in a regular grid, aligned along vector field
void generate_grid( vect2 min, vect2 max, int xsteps, int ysteps, float mutation_rate, vfield *f )
{
	int i, j;
	vect2 v,w;

	for( i=0; i<xsteps; i++ ) 
	{
		for( j=0; j<ysteps; j++ )
		{
			v.x = min.x + i * ( max.x - min.x ) / ( xsteps - 1 );
			v.y = min.y + j * ( max.y - min.y ) / ( ysteps - 1 );
			printf("subj %d %d \n", i, j);
			if( rand1() < mutation_rate ) printf("MUTANT\n");
			printf("  x = %.2f\n", v.x );
			printf("  y = %.2f\n", v.y );
			w = vfield_smooth_index( v, f );
			printf("  w.x = %.2f  w.y = %.2f\n",w.x,w.y);
			printf("  angle = %.0f\n\n", vtoa( w ) );
		}
	}
}

// creates a grouping of runs in a line
// generate_line() (stub)

// creates a random distribution of runs
// future: create a structure with the random parameters
void generate_random( int nruns, vect2 imin, vect2 imax, vect2 vmin, vect2 vmax, float ang_offset, cluster *clist)
{
	cluster *uck = clist;
	int run;
	vect2 start;

	for(run = 0; run < nruns; run++ ) {
		start = box_of_random2( vmin, vmax );
		cluster_initialize( uck );

		/* cluster_set_run( 	200, 		// number of elements in run
					start,		// starting point of run 
					0.0625, 	// initial step size
					0.99, 		// proportional change per step
					ang_offset, // motion relative to vector field
					uck );		// pointer to cluster */

		cluster_set_run( 	100, 		// number of elements in run
					start,		// starting point of run 
					0.25, 		// initial step size
					1.0, 		// proportional change per step
					ang_offset, // motion relative to vector field
					uck );		// pointer to cluster 

		cluster_set_bounds( imin, imax, uck );

		/* c_set_color( 	rand1() * 0.33 + 0.33, 		// Initial color
						rand1() * 0.005 - 0.0025, 		// Color increment
						0.5, 		// Initial brightness
						1.0, 		// Brightness proportional change per step
						uck ); */

		// cluster_set_size( true, 2.0, uck );
		cluster_set_size( true, 4.0, uck );

		uck->brightness = 0.5;
		uck->brightness_prop = 1.0;

		uck++;
	}
}

// Set up a simulated aurora
// Future: create a filetype with all the parameters 
void generate_aurora( 	float norm_frame, 
						int nruns, 
						vect2 imin, vect2 imax, 
						vect2 start, 
						float ang_offset, 
						wave_params *ang_wave, // wave_params *brightness_wave,
						palette *p,
						vfield *vf, 
						vect_fn_2d_t_params *vortex_field,
						cluster *clist,
						scene *scn)
{
	if( debug_me ) printf("generate_aurora\n");

	cluster *uck = clist;
	int run;
	float local_offset = 0.0;

	// set up color palette
	int npal = 4;
	float cindex[ npal ];
	frgb cpal[ npal ];
	float brightness;
	int i;
	vect2 perturb_start;

	for(run = 0; run < nruns; run++ ) {
		cluster_initialize( uck );

		if( debug_me ) printf("generate_aurora: perturb\n");

		// perturb starting point using perturbation field
		perturb_start = start;
		if( scn->use_perturb ) {
			for(i=0; i<scn->perturb_steps; i++ ) {
				perturb_start = vect_fn_advect( perturb_start, vortex_field, 0.0625, 0.0, norm_frame );
			}
		}

		if( debug_me ) printf("generate_aurora: cluster_set_run\n");
		cluster_set_run( 	50, 		// number of elements in run
							perturb_start,		// starting point of run 
							0.0625, 	// initial step size
							1.0, 		// proportional size change per step
							ang_offset, // motion relative to vector field
							uck );		// pointer to cluster */

		if( debug_me ) printf("generate_aurora: cluster_set_bounds\n");
		cluster_set_bounds( imin, imax, uck );

		brightness = 0.0625 + rand1() * 0.03125;
		// circle function (fade ends )
		brightness *= sqrt( 1.0 - (( run - nruns / 2.0 ) * ( run - nruns / 2.0 )) / (( nruns / 2.0 ) * ( nruns / 2.0 )));
		//brightness *= 1.0 + 0.3 * sin( (3.0 * frame / 10.0 + run) / 5.0 );
		//brightness *= 1.0 + composed_wave( run, frame, brightness_wave );
		brightness *= 1.0 + pow( rand1(), 4.0 );

		// change to custom palette
		if( debug_me ) printf("generate_aurora: cluster_set_color\n");
		cluster_set_color( 	p,								// Color palette
							0.0 + rand1() * 0.1, 			// Initial color
							0.0133, 						// Color increment
							brightness, 					// Initial brightness
							0.96, 							// Brightness proportional change per step
							uck ); 

		uck->brightness_ramp = true;
		uck->brightness_ramp_length = 5;

		// c_set_size( true, 2.0, uck );
		if( debug_me ) printf("generate_aurora: cluster_set_size\n");
		cluster_set_size( true, 2.0, uck );
		if( scn->use_wave ) local_offset = composed_wave( 1.0 * run / nruns, norm_frame, ang_wave );
		else local_offset = 0.0;

		if( debug_me ) printf("generate_aurora: vfield_advect\n");
		start = vfield_advect( start, vf, 0.0625, local_offset );

		uck++;
	}
}

/* Original aurora generation code - preserved to reconstruct in new format

	//int nruns = 1000;
	int nruns = 200;
	cluster runs[ nruns ];
	vfield vf, vf_vanish, vf_perturb, auroracentric;
	vect2 start; v_set( &start, 7.5, 7.5 );

	//v_set( &vbound1, -20.0, -20.0 * (1.0 * base.ydim) / (1.0 * base.xdim) );
	//v_set( &vbound2,  30.0,  30.0 * (1.0 * base.ydim) / (1.0 * base.xdim) );
	v_set( &vbound1,  -5.0, -5.0 );
	v_set( &vbound2,  15.0, 20.0 );

	aurora_palette( &color_palette ); // set palette 

	vfield_initialize( 1000, abs( (int)(1000 * (vbound2.y - vbound1.y) / (vbound2.x - vbound1.x) ) ), vbound1, vbound2, &vf);
	vfield_initialize( 1000, abs( (int)(1000 * (vbound2.y - vbound1.y) / (vbound2.x - vbound1.x) ) ), vbound1, vbound2, &vf_vanish);
	//vfield_initialize( 1000, abs( (int)(1000 * (vbound2.y - vbound1.y) / (vbound2.x - vbound1.x) ) ), vbound1, vbound2, &vf_perturb);
	//vfield_turbulent( 2.0, 250, false, &vf_perturb );
	//vfield_normalize(  &vf_perturb );
	//vfield_scale( 0.5, &vf );

	vect2 vanishing_point; 	v_set( &vanishing_point, 5.0, 20.0 );
	vect2 aurorigin;		v_set( &aurorigin, 5.0, 7.5);

	// Create concentric vector field for aurora beams with center above top of frame
	vfield_concentric( vanishing_point, &vf_vanish );
	vfield_scale( -1.0, &vf_vanish );
	vfield_normalize( &vf_vanish );

	vfield_concentric( aurorigin, &vf );
	vfield_normalize( &vf );
	//vfield_sum( &vf, &auroracentric, &vf );
	//vfield_normalize( &vf );

	vortex_field.n = 250;
	vortex_field.vortex_list = (vortex *)malloc( vortex_field.n * sizeof( vortex ) );
	vortex *vort = vortex_field.vortex_list;
	if( debug_me ) printf(" vortex_field.n = %d\n", vortex_field.n );
	for( i = 0; i < vortex_field.n; i++ ) {
		if( rand()%2 ) 	vort->speed_of_rotation = 1.0;
			else 		vort->speed_of_rotation = -1.0;
		vort->diameter = 2.0;
		vort->soften = 0.5;
		vort->center_of_revolution = box_of_random2( base.min, base.max );
		vort->start_offset.x = rand1() * 5.0; 	vort->start_offset.y = 0.0;
		vort->start_offset = v_rotate( vort->start_offset, rand1() * 360.0 );
		if( rand()%2 ) 	vort->revolution_velocity = 1.0;	// Should be multiple of 1.0
			else 		vort->revolution_velocity = -1.0;
		vort++;
	}

	int frame, aframe;
	float ang = 0.0;
	float ang_inc = 2.0;
	float start_step = 15.0 / nframes;

	int n_aurorae = 20;
	vect2 starts[ n_aurorae ];
	int frame_offset[ n_aurorae ];
	int ang_offset[ n_aurorae ];
	float start_angle;
	vect2 start_offset; 
	wave_params ang_wave;
	make_random_wave( 	4, 										// number of composed waves
						75.0,									// max magnitude
						6, 										// max run frequency
						5, 										// max frame frequency
						&ang_wave ); 							// pointer to wave param 

	float aurotor_speed = 0.0;
	for( frame = 0; frame < nframes; frame++ ) {
		for( a = 0; a < n_aurorae; a++ ) {

			start_angle = (a + aurotor_speed * frame / nframes) * 360.0 / n_aurorae;
			v_set( &start_offset, 3.0, 0.0 );
			start_offset = v_rotate( start_offset, start_angle );
			start = v_add( start_offset, aurorigin );

			generate_aurora( 	1.0 * frame / nframes,
								nruns, 			// Number of sub-streams in aurora
								bound1, 		// Bounds of image
								bound2, 
								start, 			// starting point
								0.0,	// offset angle
								&ang_wave,	// angle wave parameters
								&color_palette,
								&vf, 
								&vortex_field,	// perturbation field parameters
								runs, 
								&scn);		// pass pointer to scene to be added to cluster
			render_cluster_list( nruns, ang, runs, &vf_vanish, &splat, render_to, &scn );
		}

		printf("frame = %d\n",frame);

		// Clip colors in image to prevent excessive darkening
		fimage_clip( 0.0, 1.0, render_to );
		//fimage_normalize( render_to );

		if( scn.reflect_y ) fimage_reflect_y( scn.reflect_y_line, true, render_to, &result);	// reflect image if required
		if( scn.use_mask ) fimage_apply_mask( &base, &mask, &result );
		//fimage_normalize( &result );

		// save result
		sprintf( output_filename, "%s_%04d.jpg", scn.output_basename, frame);
		fimage_write_jpg( output_filename, &result );

		if( scn.use_mask ) fimage_copy_contents( &background, &background_buffer );
		else fimage_copy_contents( &base, &result );
	}

	
	for( a=0; a < n_aurorae; a++ )	{
		free_wave_params( &(ang_wave[ a ]) );
		free_wave_params( &(brightness_wave[ a ]) );
	} 

	free_wave_params( &ang_wave );
	free_vect_fn_2d_t_params( &vortex_field ); */
