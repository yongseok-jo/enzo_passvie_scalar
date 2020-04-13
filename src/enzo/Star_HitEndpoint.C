/***********************************************************************
/
/  FROM THE ATTRIBUTES, DETERMINE WHETHER THE STAR DIES
/
/  written by: John Wise
/  date:       April, 2009
/  modified1:
/
************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include "ErrorExceptions.h"
#include "macros_and_parameters.h"
#include "typedefs.h"
#include "global_data.h"
#include "Fluxes.h"
#include "GridList.h"
#include "ExternalBoundary.h"
#include "Grid.h"
#include "Hierarchy.h"
#include "TopGridData.h"
#include "LevelHierarchy.h"

#define NO_DEATH 0
#define KILL_STAR 1
#define KILL_ALL 2

#define GMC_TEST_5  //##### for 121512_dGF-RTF
#ifdef GMC_TEST_5
int GetUnits(float *DensityUnits, float *LengthUnits,
	     float *TemperatureUnits, float *TimeUnits,
	     float *VelocityUnits, FLOAT Time);
#endif /* GMC_TEST */

int Star::HitEndpoint(FLOAT Time)
{

  const float TypeIILowerMass = 11, TypeIIUpperMass = 40;
  const float PISNLowerMass = 140, PISNUpperMass = 260;

  /* First check if the star's past its lifetime and then check other
     constrains based on its star type */

  int result = NO_DEATH;
  if ((Time > this->BirthTime + this->LifeTime) && this->type >=0)
    result = KILL_STAR;
  else
    return result;

  switch (this->type) {

  case PopIII:
    // If a Pop III star is going supernova, only kill it after it has
    // applied its feedback sphere
    if ((this->Mass >= PISNLowerMass && this->Mass <= PISNUpperMass) ||
	(this->Mass >= TypeIILowerMass && this->Mass <= TypeIIUpperMass)) {

      // Needs to be non-zero (multiply by a small number to retain
      // memory of mass)
      if (this->FeedbackFlag == DEATH) {
	this->Mass *= tiny_number;  


	// Set lifetime so the time of death is exactly now.
	this->LifeTime = Time - this->BirthTime;

	//this->FeedbackFlag = NO_FEEDBACK;
	result = KILL_STAR;
	//result = NO_DEATH;
      } else {
	result = NO_DEATH;
      }

    // Check mass: Don't want to kill tracer SN particles formed
    // (above) in the previous timesteps.

    } else if (this->Mass > 1e-9) {
      // Turn particle into a black hole (either radiative or tracer)
      if (PopIIIBlackHoles) {
	this->type = BlackHole;
	this->LifeTime = huge_number;
	this->FeedbackFlag = NO_FEEDBACK;
	result = NO_DEATH;
      } else {
	this->type = PARTICLE_TYPE_DARK_MATTER;
	result = KILL_STAR;
      }
    } else // SN tracers (must refine)
      result = NO_DEATH;
    break;
    
  case PopII:
    break;

  case BlackHole:
    break;
    
  case MBH:
#ifdef GMC_TEST_5  //#####  THIS HAD NO IMPACT...  You stupid...  (check the first if statement in this file -> particle type stayed the same but never listed as Star after first timestep, 12/2012)
    //GMC's lifetime as a radiation source = 10 t_dyn
    //if (Time > this->BirthTime + 10*this->LifeTime)
    //      result = NO_DEATH;

    //GMC's lifetime as a radiation source = 6 Myrs -> 100 Myrs
    float DensityUnits, LengthUnits, TemperatureUnits, TimeUnits,
      VelocityUnits;
    GetUnits(&DensityUnits, &LengthUnits, &TemperatureUnits,
	     &TimeUnits, &VelocityUnits, Time);
    if (Time > this->BirthTime + 100e6*3.15e7/TimeUnits)
      this->type = PARTICLE_TYPE_STAR;  
      result = NO_DEATH;
#endif /* GMC_TEST */
    break;

  case PopIII_CF:
    break;

  } // ENDSWITCH

  return result;
}
