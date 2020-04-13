/***********************************************************************
/
/  CREATES STAR PARTICLES FROM EXISTING PARTICLES
/
/  written by: John Wise
/  date:       March, 2009
/  modified1:
/
/  NOTES:  negative types mark particles that have just been before 
/          and not been converted into a star particle.
/
************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ErrorExceptions.h"
#include "macros_and_parameters.h"
#include "typedefs.h"
#include "global_data.h"
#include "Fluxes.h"
#include "GridList.h"
#include "ExternalBoundary.h"
#include "Grid.h"

void InsertStarAfter(Star * &Node, Star * &NewNode);
int GetUnits(float *DensityUnits, float *LengthUnits,
             float *TemperatureUnits, float *TimeUnits,
             float *VelocityUnits, FLOAT Time);

int grid::FindNewStarParticles(int level)
{

  if (MyProcessorNumber != ProcessorNumber)
    return SUCCESS;

  if (NumberOfParticles == 0)
    return SUCCESS;

  int i;
  Star *NewStar, *cstar;
  bool exists;

  for (i = 0; i < NumberOfParticles; i++)
    if (ParticleType[i] == -PARTICLE_TYPE_SINGLE_STAR ||
	ParticleType[i] == -PARTICLE_TYPE_BLACK_HOLE ||
	ParticleType[i] == -PARTICLE_TYPE_CLUSTER ||
	ParticleType[i] == -PARTICLE_TYPE_COLOR_STAR ||
	ParticleType[i] == -PARTICLE_TYPE_SIMPLE_SOURCE ||
	ABS(ParticleType[i]) == PARTICLE_TYPE_MBH) {

      // Check if it already exists (wasn't activated on the last
      // timestep, usually because of insufficient mass)
      exists = false;
      for (cstar = Stars; cstar; cstar = cstar->NextStar)
	if (cstar->Identifier == ParticleNumber[i]) {
	  exists = true;
	  break;
	}

      if (!exists) {

#define GMC_TEST //##### identify as Stars only without a certain radius from a center
#ifdef GMC_TEST
	float dr, result = 0, fixed_center[MAX_DIMENSION];

	/* The GMC_TEST is totally ad hoc at this point, very specific to 
	   a test dataset of an isolated galaxy - Ji-hoon Kim, Jul.2011 */
	// fixed_center[0] = 0.4993;
	// fixed_center[1] = 0.5000;
	// fixed_center[2] = 0.4995;  
	
	fixed_center[0] = 0.503059;
	fixed_center[1] = 0.495329; 
	fixed_center[2] = 0.476030;
	
	for (int dim = 0; dim < MAX_DIMENSION; dim++) {
	  dr = ParticlePosition[dim][i] - fixed_center[dim];
	  result += dr*dr;
	}

	/* Only when the stars outside the 2.5 kpc of the galactic center
	   consider them as radiating Star particles - Ji-hoon Kim, Jul.2011 */
	if (ParticleType[i] == PARTICLE_TYPE_MBH &&
	    sqrt(result) > 0.01) 
	    // sqrt(result) < -0.0025) 
	  goto next;

	//	fprintf(stdout, "grid::FNSP - this star (ID:%d) is in, with t_dyn = %g\n", 
	//	ParticleNumber[i], ParticleAttribute[1][i]);
//		ParticleNumber[i], ParticlePosition[0][i], ParticlePosition[1][i], ParticlePosition[2][i]); 
#endif /* GMC_TEST */


#define NO_GMC_TEST_2 //##### identify as Stars only within a certain radius from a rotating center
#ifdef GMC_TEST_2
	const double sec_per_year = 3.1557e7;
	float dr, result = 0, initial_time, rotating_velocity, rotating_radius, 
	  rotating_center[MAX_DIMENSION], initial_angle;
	float MassConversion, DensityUnits, LengthUnits, TemperatureUnits, 
	  TimeUnits, VelocityUnits;
	GetUnits(&DensityUnits, &LengthUnits, &TemperatureUnits,
		 &TimeUnits, &VelocityUnits, this->Time);

	/* The GMC_TEST is totally ad hoc at this point, very specific to 
	   a test dataset of an isolated galaxy - Ji-hoon Kim, Jul.2011 */
	initial_time = 0.014390;
	rotating_velocity = 4.0/50.0/sqrt(2)/1.5e6; // in radian/yr, assuming this is constant with radius
	
// 	rotating_radius = 0.005*sqrt(2); // in code unit
//      initial_angle = 135; // in degree, initially this should give (0.4950, 0.5050, 0.5000)

	rotating_radius = 0.0049; // in code unit
        initial_angle = 294; // in degree, initially this should give (0.5020, 0.4955, 0.5000)
	

	rotating_center[0] = 0.5000 + rotating_radius * cos(initial_angle * PI/180 
			     + rotating_velocity * (this->Time-initial_time) * TimeUnits / sec_per_year);
	rotating_center[1] = 0.5000 + rotating_radius * sin(initial_angle * PI/180 
			     + rotating_velocity * (this->Time-initial_time) * TimeUnits / sec_per_year);
	rotating_center[2] = 0.5000;  
	
	for (int dim = 0; dim < MAX_DIMENSION; dim++) {
	  dr = ParticlePosition[dim][i] - rotating_center[dim];
	  result += dr*dr;
	}

	/* Only when the stars are within 2 kpc of the rotating_center
	   consider them as radiating Star particles - Ji-hoon Kim, Jul.2011 */
	if (ParticleType[i] == PARTICLE_TYPE_MBH &&
	    sqrt(result) > 0.002) 
	  goto next;

	fprintf(stdout, "grid::FNSP - this star (ID:%d) is in!\n", ParticleNumber[i]); 
	fprintf(stdout, "(this->Time-initial_time) * TimeUnits / sec_per_year = %g\n", (this->Time-initial_time) * TimeUnits / sec_per_year);
// 	fprintf(stdout, "cos(arg) = %g\n", cos(135 * PI/180 + rotating_velocity * (this->Time-initial_time) * TimeUnits / sec_per_year));
// 	fprintf(stdout, "ParticlePosition = (%g, %g, %g)\n", ParticlePosition[0][i], ParticlePosition[1][i], ParticlePosition[2][i]);
// 	fprintf(stdout, "rotating_center  = (%g, %g, %g)\n", rotating_center[0], rotating_center[1], rotating_center[2]);
// 	fprintf(stdout, "result = %g, sqrt(result) = %g\n", result, sqrt(result));
#endif /* GMC_TEST_2 */

	NewStar = new Star(this, i, level);

	/* If using an IMF for Pop III stars, assign the mass after
	   merging new (massless) star particles to avoid unnecessary
	   calls to the IMF. */

	if (ParticleType[i] == -PARTICLE_TYPE_SINGLE_STAR)
	  if (PopIIIInitialMassFunction == FALSE)
	    NewStar->AssignFinalMass(PopIIIStarMass);
	if (ParticleType[i] == -PARTICLE_TYPE_SIMPLE_SOURCE) 
	  NewStar->AssignFinalMass(PopIIIStarMass);
	InsertStarAfter(Stars, NewStar);
	NumberOfStars++;

#ifdef GMC_TEST
      next:
#endif /* GMC_TEST */
      }

    }

  return SUCCESS;

}
