/***********************************************************************
/
/  DETERMINES WHETHER THE STAR PARTICLE IS RADIATIVE
/
/  written by: John Wise
/  date:       March, 2009
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

bool Star::IsARadiationSource(FLOAT Time)
{

  int i;
  bool *rules, result = true;

  /* To add rules, you must also modify NumberOfRules. */

  const int NumberOfRules = 4;
  rules = new bool[NumberOfRules];

  for (i = 0; i < NumberOfRules; i++) 
    rules[i] = false; 

  /*******************************************************************
     Below are the multiple definitions for a radiation source.  If
     all of the rules are met, the star particle is a radiation
     source. 
  ********************************************************************/

  // Particles only marked for nothing or continuous supernova
  rules[0] = (FeedbackFlag == NO_FEEDBACK || 
	      FeedbackFlag == CONT_SUPERNOVA ||
	      FeedbackFlag == MBH_THERMAL ||
	      FeedbackFlag == MBH_JETS);
  
  // Living
#define GMC_TEST_6 //#####  for 080112_CHaRGe/1e13vb_aRTF
#ifdef GMC_TEST_6
  rules[1] = true;
  //rules[1] = BirthTime < 0; //##### quick test on 03/16/2017 for /u/jkim30/work/enzo-runs/080112_CHaRGe/ic.enzo_1e13vb_nocv_z=6_MBH_noStarRad/DD0261; see your note email in the Draft folder
#else
  rules[1] = (Time >= BirthTime && Time <= BirthTime+LifeTime && type > 0);
#endif /* GMC_TEST_6 */

  // Non-zero BH accretion (usually accretion_rate[] here is NULL - Ji-hoon Kim Sep.2009)
#define GMC_TEST
#ifdef GMC_TEST
  rules[2] = (type == MBH);
#else
  if ((type == BlackHole || type == MBH) && naccretions > 0)
    rules[2] = (accretion_rate[0] > tiny_number); 
  else
    rules[2] = true;
#endif /* GMC_TEST */

  // Non-zero mass
  rules[3] = (Mass > tiny_number);

  /******************** END RULES ********************/

  for (i = 0; i < NumberOfRules; i++)
    result &= rules[i];

  delete [] rules;

  return result;

}
