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

#define RESET_BH_LIFETIMES
#define NO_RESET_MBH_MASS  //#####

int grid::FindAllStarParticles(int level)
{

  if (MyProcessorNumber != ProcessorNumber)
    return SUCCESS;

  if (NumberOfParticles == 0)
    return SUCCESS;

  int i, StarType;
  Star *NewStar;
  
  /* Read only active star particles.  Unborn stars will be read later
     in grid::FindNewStarParticles. */
  /*
#ifdef GMC_TEST
  int count=0;
  float MBHPosition[MAX_DIMENSION];
  for (i = 0; i < NumberOfParticles; i++){
    if (ParticleMass[i] > 1e5) {
      for (int dim = 0; dim < MAX_DIMENSION; dim++){
  	MBHPosition[dim] = ParticlePosition[i][dim];
	printf("%f\n", MBHPosition[dim]);
	count++;
      }
    }
    continue;
  }
  printf("The number of MBH is %d\n", count);
   
  #endif */ /* GMC_TEST */

  for (i = 0; i < NumberOfParticles; i++) {
    //StarType = abs(ParticleType[i]);
    if (ParticleType[i] == PARTICLE_TYPE_SINGLE_STAR ||
	ParticleType[i] == PARTICLE_TYPE_BLACK_HOLE ||
	ParticleType[i] == PARTICLE_TYPE_CLUSTER ||
        ParticleType[i] == PARTICLE_TYPE_COLOR_STAR ||
	ParticleType[i] == PARTICLE_TYPE_MBH ||
	ParticleType[i] == PARTICLE_TYPE_SIMPLE_SOURCE) {

      if (this->Time >= ParticleAttribute[0][i] &&
	  this->Time <= ParticleAttribute[0][i]+ParticleAttribute[1][i]) {
	  
#ifdef RESET_BH_LIFETIMES // Make BH lifetimes "infinite"
	if (ParticleType[i] == PARTICLE_TYPE_BLACK_HOLE &&
	    ParticleAttribute[1][i] < 1)
	  ParticleAttribute[1][i] = huge_number;
#endif /* RESET_BH_LIFETIMES */

#ifdef RESET_MBH_MASS // Edit MBH Mass; only for test purpose
	const double Msun = 1.989e33;
	float MassConversion, DensityUnits, LengthUnits, TemperatureUnits, 
	  TimeUnits, VelocityUnits;
	GetUnits(&DensityUnits, &LengthUnits, &TemperatureUnits,
		 &TimeUnits, &VelocityUnits, this->Time);

	double dx = LengthUnits * this->CellWidth[0][0];
	MassConversion = (float) (dx*dx*dx * double(DensityUnits) / Msun);

	if (ParticleType[i] == PARTICLE_TYPE_MBH)
	  ParticleMass[i] = 1.0e6 / MassConversion;
#endif /* RESET_MBH_MASS */  

#define NO_GMC_TEST_REMOVE_EXISTING //#####
#ifdef GMC_TEST_REMOVE_EXISTING
	/* remove the existing MBH particle at the beginning of the simulation */
	if (ParticleType[i] == PARTICLE_TYPE_MBH) //&& ParticleNumber[i] == 880785)
	  fprintf(stdout, "grid::FASP: particle ID %d found and removed? (0:no/1:yes) = %d\n", 
		  ParticleNumber[i], this->RemoveParticle(ParticleNumber[i], true));
#endif /* GMC_TEST_REMOVE_EXISTING */

#define GMC_TEST //##### identify as Stars only without a certain radius from a center
#ifdef GMC_TEST
	float dr, result = 0; //, fixed_center[MAX_DIMENSION];

	/* The GMC_TEST is totally ad hoc at this point, very specific to 
	   a test dataset of an isolated galaxy - Ji-hoon Kim, Jul.2011 */
	// fixed_center[0] = 0.4993;
	// fixed_center[1] = 0.5000;
	// fixed_center[2] = 0.4995;  
	
        // fixed_center[0] = 0.503059;  // when at DD0195; check your AGORA+RTF note
	// fixed_center[1] = 0.495329;
	// fixed_center[2] = 0.476030;

	// fixed_center[0] = 0.5027207;  // when at DD0240; check your AGORA+RTF note
	// fixed_center[1] = 0.4958971; 
	// fixed_center[2] = 0.4760798;

	//fixed_center[0] = 0.50240872;           // when at DD0410 by Jo
	//fixed_center[1] = 0.49630405275787326;
	//fixed_center[2] = 0.476217010185609;  

	//fixed_center[0] = 0.50267965972893487;           // when at DD0261 by Jo
	//fixed_center[1] = 0.49595975956918104;
	//fixed_center[2] = 0.47608792531275995;           //it doesn't need anymore since we can trace the position of MBH.
	

	for (int dim = 0; dim < MAX_DIMENSION; dim++) {
	  dr = ParticlePosition[dim][i] - bh_pos[dim]; //fixed_center[dim];  by Jo
	  result += dr*dr;
	}

	/* Only when the stars outside the 2.5 kpc of the galactic center
	   consider them as radiating Star particles - Ji-hoon Kim, Jul.2011 */
	if (ParticleType[i] == PARTICLE_TYPE_MBH &&
	    //sqrt(result) > 0.002) // when at DD0195; check your AGORA+RTF note
	    //sqrt(result) > 0.0015) // when at DD0240; check your AGORA+RTF note
	    sqrt(result) > 0.0005)// when at DD0261 by Jo
	    //sqrt(result) > 0.000005)// when at DD0410 by Jo tentative
	  // sqrt(result) < -0.0025) 
	  goto next;

	//	fprintf(stdout, "grid::FNSP - this star (ID:%d) is in, with t_dyn = %g\n", 
	//	ParticleNumber[i], ParticleAttribute[1][i]);
//		ParticleNumber[i], ParticlePosition[0][i], ParticlePosition[1][i], ParticlePosition[2][i]); 
#endif /* GMC_TEST */

#define NO_GMC_TEST_2
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

	/* Only when the stars are within 1 kpc of the rotating_center
	   consider them as radiating Star particles - Ji-hoon Kim, Jul.2011 */
	if (ParticleType[i] == PARTICLE_TYPE_MBH &&
	    sqrt(result) > 0.002) 
	  goto next;
#endif /* GMC_TEST_2 */

	NewStar = new Star(this, i, level);
	InsertStarAfter(Stars, NewStar);
	NumberOfStars++;

	/* For MBHFeedback = 2 to 6 (FeedbackFlag=MBH_JETS), you need
	   the angular momentum; if no file to read in, assume zero
	   angular momentum accreted so far.  -Ji-hoon Kim, Nov.2009 */

	if((MBHFeedback >= 2 && MBHFeedback <= 6) && 
	   ParticleType[i] == PARTICLE_TYPE_MBH) {
	  NewStar->AssignAccretedAngularMomentum();
	  printf("MBH particle info (for MBHFeedback = 2 to 6, check angular momentum): \n"); 
	  NewStar->PrintInfo();
	}

#ifdef GMC_TEST
      next:
#endif /* GMC_TEST */

      } // ENDIF during lifetime
    } // ENDIF star
  } // ENDFOR particles
  
  return SUCCESS;

}
